// Loop-SaboteurXXL's own offline test (x86, under ASan/UBSan from test/run_tests.sh): the engine glue and the
// vendored processor together, through the same mpc_engine_t calls the wrapper makes.
//
//   engine_test                 run every check; exit status 0 = all passed
//   engine_test --values FILE   write every parameter's value and text for a demo session (tools/preview.py)
//   engine_test --wav DIR       also write a few renders to listen to
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <vector>

extern "C" {
#include "engine.h"
int lsx_param_count (void);
const char* lsx_param_key (int i);
}

static const mpc_engine_t* E;
static int g_fail = 0;
#define CHECK(c, ...) do { if (! (c)) { std::printf ("FAIL %s:%d: ", __FILE__, __LINE__); std::printf (__VA_ARGS__); std::printf ("\n"); ++g_fail; } } while (0)

static std::string get (void* x, const char* k)
{
    std::vector<char> b (1 << 20);
    const int n = E->get_param (x, k, b.data(), (int) b.size());
    return n > 0 ? std::string (b.data()) : std::string();
}
static double getv (void* x, const char* k) { return std::atof (get (x, k).c_str()); }
static void set (void* x, const char* k, double v) { char b[48]; std::snprintf (b, sizeof b, "%.9g", v); E->set_param (x, k, b); }
static void tap (void* x, const char* k) { E->set_param (x, k, "1"); E->set_param (x, k, "0"); }

// a drum-ish loop: a kick (decaying sine) on each beat, a noisy hat on the off-8ths, at 120 BPM
static void source (std::vector<int16_t>& buf, long long frame0, int frames)
{
    for (int i = 0; i < frames; ++i)
    {
        const long long f = frame0 + i;
        const double t = (double) f / 44100.0;
        const double beat = std::fmod (t, 0.5), half = std::fmod (t + 0.25, 0.5);
        double s = 0.6 * std::sin (2 * M_PI * 60.0 * beat) * std::exp (-beat * 18.0);
        s += 0.15 * (((f * 1103515245 + 12345) >> 16 & 0xffff) / 32768.0 - 1.0) * std::exp (-half * 60.0);
        const int16_t v = (int16_t) std::lround (s * 30000.0);
        buf[2 * i] = v;
        buf[2 * i + 1] = v;
    }
}

struct Run
{
    double peak = 0, diff = 0;
    bool finite = true;
    std::vector<int16_t> out;
};

// play `seconds` at 120 BPM from ppq0 (playing), 128-frame blocks, the way the wrapper calls the engine
static Run play (void* x, double seconds, bool playing = true, double ppq0 = 0.0, bool keep = false, double loopBeats = 0)
{
    Run r;
    std::vector<int16_t> in (256), out (256);
    const int blocks = (int) (seconds * 44100.0 / 128.0);
    double ppq = ppq0;
    for (int b = 0; b < blocks; ++b)
    {
        E->song_pos (x, ppq, 120.0, playing ? 1 : 0, 128);
        source (in, (long long) b * 128, 128);
        E->process (x, in.data(), out.data(), 128);
        for (int i = 0; i < 256; ++i)
        {
            r.peak = std::fmax (r.peak, std::fabs (out[i] / 32768.0));
            r.diff += std::fabs ((out[i] - in[i]) / 32768.0);
        }
        if (keep) r.out.insert (r.out.end(), out.begin(), out.end());
        if (playing) ppq += 128.0 * 120.0 / 60.0 / 44100.0;
        if (loopBeats > 0 && ppq >= ppq0 + loopBeats) ppq -= loopBeats;
    }
    return r;
}

static void wav (const std::string& path, const std::vector<int16_t>& s)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    if (! f) return;
    const unsigned data = (unsigned) (s.size() * 2), riff = 36 + data, rate = 44100, byteRate = rate * 4;
    const unsigned short fmt = 1, ch = 2, align = 4, bits = 16;
    const unsigned fmtLen = 16;
    std::fwrite ("RIFF", 1, 4, f); std::fwrite (&riff, 4, 1, f); std::fwrite ("WAVEfmt ", 1, 8, f);
    std::fwrite (&fmtLen, 4, 1, f); std::fwrite (&fmt, 2, 1, f); std::fwrite (&ch, 2, 1, f); std::fwrite (&rate, 4, 1, f);
    std::fwrite (&byteRate, 4, 1, f); std::fwrite (&align, 2, 1, f); std::fwrite (&bits, 2, 1, f);
    std::fwrite ("data", 1, 4, f); std::fwrite (&data, 4, 1, f); std::fwrite (s.data(), 2, s.size(), f);
    std::fclose (f);
}

// a demo session: Acts from the factory presets, a pattern with every kind of mark
static void demo (void* x)
{
    const int presets[8] = { 1, 4, 8, 5, 14, 2, 17, 10 };
    for (int a = 0; a < 8; ++a)
    {
        set (x, "act", a);
        set (x, "preset", presets[a]);
    }
    set (x, "act", 0);
    set (x, "seqon", 1);
    const int pat[16] = { 0, -1, 5, -1, 0, 2, -1, -1, 0, -1, 5, 5, -1, 1, 5, -1 };
    for (int i = 0; i < 16; ++i)
        if (pat[i] >= 0)
        {
            set (x, "act", pat[i]);
            tap (x, ("stamp" + std::to_string (i + 1)).c_str());
        }
    set (x, "act", 5);
    set (x, "step", 10); set (x, "st_cond", 1);
    set (x, "step", 8); set (x, "st_ratchet", 3);
    set (x, "step", 16); set (x, "st_freeze", 1);
    set (x, "step", 13); set (x, "st_mute", 1);
    set (x, "step", 10); set (x, "lockedit", 1); set (x, "drive", 0.6); set (x, "lockedit", 0);
    set (x, "act", 0);
}

int main (int argc, char** argv)
{
    E = mpc_engine();
    std::string wavDir, valuesFile;
    for (int i = 1; i + 1 < argc; ++i)
    {
        if (! std::strcmp (argv[i], "--wav")) wavDir = argv[i + 1];
        if (! std::strcmp (argv[i], "--values")) valuesFile = argv[i + 1];
    }

    if (! valuesFile.empty())
    {
        void* x = E->create (nullptr);
        demo (x);
        set (x, "mod", 0);
        FILE* f = std::fopen (valuesFile.c_str(), "w");
        std::fprintf (f, "{\n");
        const int n = lsx_param_count();
        for (int i = 0; i < n; ++i)
        {
            const char* k = lsx_param_key (i);
            std::string t = get (x, (std::string (k) + "_display").c_str());
            if (! std::strcmp (k, "readout")) t = get (x, "readout");
            std::string esc;
            for (char c : t) { if (c == '"' || c == '\\') esc += '\\'; esc += c; }
            std::fprintf (f, " \"%s\": [%s, \"%s\"]%s\n", k, std::strcmp (k, "readout") ? get (x, k).c_str() : "0",
                          esc.c_str(), i + 1 < n ? "," : "");
        }
        std::fprintf (f, "}\n");
        std::fclose (f);
        E->destroy (x);
        return 0;
    }

    // 1. every parameter answers, and options / ranges round-trip
    {
        void* x = E->create (nullptr);
        const int n = lsx_param_count();
        CHECK (n > 150, "parameter count %d", n);
        for (int i = 0; i < n; ++i)
        {
            const char* k = lsx_param_key (i);
            if (! std::strcmp (k, "readout")) { CHECK (! get (x, "readout").empty(), "readout text"); continue; }
            CHECK (! get (x, k).empty(), "%s has no value", k);
            CHECK (! get (x, (std::string (k) + "_display").c_str()).empty(), "%s has no text", k);
        }
        set (x, "division", 4); CHECK (getv (x, "division") == 4, "division");
        CHECK (get (x, "division_display") == "1/8", "division text %s", get (x, "division_display").c_str());
        set (x, "tone", 0.5); CHECK (get (x, "tone_display") == "FLAT", "tone FLAT");
        set (x, "crunch", 0.0); CHECK (get (x, "crunch_display") == "OFF", "bits OFF");
        set (x, "crunch", 1.0); CHECK (get (x, "crunch_display") == "2-bit", "bits 2-bit: %s", get (x, "crunch_display").c_str());
        set (x, "gate", -0.45); CHECK (get (x, "gate_display") == "-45% Soft", "shape: %s", get (x, "gate_display").c_str());
        set (x, "pitch", 7); CHECK (get (x, "pitch_display") == "+7 st", "pitch: %s", get (x, "pitch_display").c_str());
        set (x, "m_attack", 0.5); CHECK (std::fabs (getv (x, "m_attack") - 0.5) < 1e-3, "attack round trip");
        set (x, "e_xover", 0.25); CHECK (std::fabs (getv (x, "e_xover") - 0.25) < 1e-3, "crossover round trip");
        set (x, "m_target", 5); CHECK (getv (x, "m_target") == 5, "mod target round trip");
        // the Act's knobs follow the Act
        set (x, "act", 0); set (x, "drive", 0.25);
        set (x, "act", 1); CHECK (getv (x, "drive") == 0.0, "act B drive %g", getv (x, "drive"));
        set (x, "drive", 0.75);
        set (x, "act", 0); CHECK (std::fabs (getv (x, "drive") - 0.25) < 1e-6, "act A drive back %g", getv (x, "drive"));
        // a preset lands in the Act and shows its name
        set (x, "preset", 1);
        CHECK (get (x, "preset_display") == "Jungle Stretch", "preset %s", get (x, "preset_display").c_str());
        set (x, "drive", 0.11);
        CHECK (get (x, "preset_display") == "-", "edited preset %s", get (x, "preset_display").c_str());
        // LOCK EDIT writes a p-lock, an echo of the same value does not
        set (x, "step", 5); set (x, "lockedit", 1);
        const double before = getv (x, "res");
        set (x, "res", before);   // a host echo
        CHECK (((int) getv (x, "dots5") & 1) == 0, "echo made a p-lock");
        set (x, "res", 0.5);
        CHECK (((int) getv (x, "dots5") & 1) == 1, "p-lock mark");
        CHECK (std::fabs (getv (x, "res") - 0.5) < 1e-6, "lock value shown");
        set (x, "lockedit", 0);
        CHECK (std::fabs (getv (x, "res") - before) < 1e-6, "knob value back after LOCK EDIT");
        // stamping
        set (x, "act", 2); set (x, "page", 0);
        tap (x, "stamp3");
        CHECK (getv (x, "cell3") == 3, "cell3 = C (%g)", getv (x, "cell3"));
        tap (x, "stamp3");
        CHECK (getv (x, "cell3") == 0, "cell3 cleared");
        set (x, "length", 8);
        CHECK (getv (x, "cell10") == 18, "past the length (%g)", getv (x, "cell10"));
        E->destroy (x);
    }

    // 2. freehand: the chopper fires on the grid and changes the sound; nothing runs away
    {
        void* x = E->create (nullptr);
        set (x, "preset", 1);   // Jungle Stretch on Act A
        Run r = play (x, 8.0, true, 0.0, ! wavDir.empty());
        CHECK (r.finite && r.peak <= 1.0, "peak %g", r.peak);
        CHECK (r.diff > 100.0, "the chopper changed nothing (diff %g)", r.diff);
        if (! wavDir.empty()) wav (wavDir + "/freehand_jungle_stretch.wav", r.out);
        // stopped: dry passes through untouched apart from the safety limiter
        Run s = play (x, 1.0, false);
        CHECK (s.peak <= 1.0, "stopped peak %g", s.peak);
        E->destroy (x);
    }

    // 3. the sequencer steps on MPC's grid, also across a loop wrap, with every Act and mark in play
    {
        void* x = E->create (nullptr);
        demo (x);
        Run r = play (x, 16.0, true, 0.0, ! wavDir.empty(), 8.0);
        CHECK (r.peak <= 1.0, "seq peak %g", r.peak);
        CHECK (r.diff > 100.0, "the sequencer changed nothing");
        if (! wavDir.empty()) wav (wavDir + "/sequencer_demo.wav", r.out);
        // every output stage, every Act
        for (int st = 0; st < 11; ++st)
        {
            for (int a = 0; a < 8; ++a) { set (x, "act", a); set (x, "e_stage", st); set (x, "e_mix", 1.0); }
            Run o = play (x, 1.0);
            CHECK (o.peak <= 1.0 && o.finite, "stage %d peak %g", st, o.peak);
        }
        E->destroy (x);
    }

    // 4. state: a big session survives the chunk round trip exactly
    {
        void* x = E->create (nullptr);
        demo (x);
        for (int s = 1; s <= 64; ++s)
        {
            set (x, "step", s);
            set (x, "lockedit", 1);
            set (x, "pitch", (s % 13) - 6.0);
            set (x, "drive", (s % 10) / 10.0);
            set (x, "tone", (s % 7) / 7.0);
            for (const char* k : { "fold", "smear", "res", "chaos", "reverse", "stretch" }) set (x, k, ((s * 7) % 11) / 11.0);
            set (x, "lockedit", 0);
        }
        for (int a = 0; a < 8; ++a)
            for (int m = 0; m < 4; ++m)
            {
                set (x, "act", a); set (x, "mod", m);
                set (x, "m_shape", (a + m) % 7); set (x, "m_rate", 1 + (a * 3 + m) % 12); set (x, "m_depth", 0.3);
                set (x, "m_target", 1 + (a + 5 * m) % 29);
            }
        const std::string st = get (x, "state");
        CHECK (st.size() > 8192, "state %zu bytes (the test wants a big one)", st.size());
        void* y = E->create (nullptr);
        E->set_param (y, "state", st.c_str());
        CHECK (get (y, "state") == st, "state round trip differs");
        for (const char* k : { "drive", "act", "seqon", "length", "m_shape", "cell1", "dots10", "e_stage" })
            CHECK (get (y, k) == get (x, k), "%s after restore: %s vs %s", k, get (y, k).c_str(), get (x, k).c_str());
        Run r = play (y, 4.0);
        CHECK (r.peak <= 1.0, "restored peak %g", r.peak);
        E->destroy (y);

        // 5. banks: save, change everything, load back
        char tmpl[] = "/tmp/lsxxl_banksXXXXXX";
        const char* dir = mkdtemp (tmpl);
        void* z = E->create (dir);
        E->set_param (z, "state", st.c_str());
        set (z, "bank", 3);
        tap (z, "bank_save");
        CHECK (get (z, "readout").find ("saved") != std::string::npos, "bank save: %s", get (z, "readout").c_str());
        const std::string saved = get (z, "state");
        tap (z, "act_clearall");
        tap (z, "seq_clear");
        set (z, "page", 0);
        CHECK (getv (z, "cell1") == 0, "cleared (%g)", getv (z, "cell1"));
        tap (z, "bank_load");
        CHECK (get (z, "readout").find ("loaded") != std::string::npos, "bank load: %s", get (z, "readout").c_str());
        set (x, "page", 0);
        CHECK (get (z, "cell1") == get (x, "cell1"), "bank pattern back");
        set (z, "bank", 7);
        tap (z, "bank_load");
        CHECK (get (z, "readout").find ("empty") != std::string::npos, "empty bank: %s", get (z, "readout").c_str());
        E->destroy (z);
        std::string cmd = std::string ("rm -rf '") + dir + "'";
        if (std::system (cmd.c_str()) != 0) {}
        E->destroy (x);
    }

    // 6. undo / page copy-paste / Act copy-paste / panic
    {
        void* x = E->create (nullptr);
        demo (x);
        const std::string c1 = get (x, "cell1");
        tap (x, "seq_clear");
        CHECK (getv (x, "cell1") == 0, "seq clear");
        tap (x, "undo");
        CHECK (get (x, "cell1") == c1, "undo");
        set (x, "page", 0); tap (x, "pg_copy"); set (x, "page", 2); tap (x, "pg_paste");
        CHECK ((int) getv (x, "cell1") % 9 == std::atoi (c1.c_str()) % 9, "page paste");
        set (x, "act", 1); tap (x, "act_copy"); const std::string d = get (x, "drive");
        set (x, "act", 7); tap (x, "act_paste");
        CHECK (get (x, "drive") == d, "act paste drive %s vs %s", get (x, "drive").c_str(), d.c_str());
        for (int m = 0; m < 19; ++m) { set (x, "rnd_mode", m); tap (x, "rnd_go"); Run r = play (x, 0.5); CHECK (r.peak <= 1.0, "random %d", m); }
        tap (x, "panic");
        Run r = play (x, 0.5);
        CHECK (r.peak <= 1.0, "after panic");
        E->destroy (x);
    }

    // 7. speed: a heavy session on this machine (the Force is several times slower: see BENCH in the README)
    {
        void* x = E->create (nullptr);
        demo (x);
        for (int a = 0; a < 8; ++a)
            for (int m = 0; m < 4; ++m)
            {
                set (x, "act", a); set (x, "mod", m);
                set (x, "m_shape", m % 2 ? 0 : 6); set (x, "m_rate", 5); set (x, "m_depth", 0.4); set (x, "m_target", 8);
            }
        for (int a = 0; a < 8; ++a) { set (x, "act", a); set (x, "e_stage", 3); set (x, "e_mix", 0.8); set (x, "stretch", 0.6); set (x, "smear", 0.5); }
        const auto t0 = std::chrono::steady_clock::now();
        play (x, 30.0, true, 0.0, false, 8.0);
        const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        std::printf ("speed: 30 s of audio in %.2f s (%.1fx real time, %.1f%% of a core)\n", secs, 30.0 / secs, secs / 30.0 * 100.0);
        E->destroy (x);
    }

    std::printf (g_fail ? "FAILED: %d check(s)\n" : "PASSED\n", g_fail);
    return g_fail ? 1 : 0;
}
