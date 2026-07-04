#pragma once

#include <juce_core/juce_core.h>

// Factory presets, transcribed from "Tableau III - Liste des timbres" of the
// Ondioline construction manual. Each entry names the levers to lower and the
// register (cle d'octaves position). Where the manual lists alternatives, the
// first combination given is used.
namespace ondaire::presets
{
    struct Preset
    {
        const char* name;
        const char* levers;     // e.g. "AF" or "FGIJ"; "1"/"2" stand for V1/V2
        int         registre;   // 1..4
        float       percDecay;  // seconds; only meaningful when P is engaged
        float       noise;      // souffle level
        bool        poly;       // authentic patches stay mono
    };

    // Lever letters: A B C D E F G H I J K M P, plus 1 = V1, 2 = V2, W.
    inline constexpr Preset list[] =
    {
        { "Init (Tops)",            "",        3, 0.6f,  0.0f,  false },
        { "Violon",                 "AF",      3, 0.6f,  0.0f,  false },
        { "Violon Sourdine",        "AFH2W",   4, 0.6f,  0.0f,  false },
        { "Violoncelle",            "AF1",     1, 0.6f,  0.0f,  false },
        { "Saxophone Alto",         "CGIJ",    3, 0.6f,  0.03f, false },
        { "Saxophone Tenor",        "CGK",     2, 0.6f,  0.03f, false },
        { "Trompette Jazz",         "GIJ",     3, 0.6f,  0.0f,  false },
        { "Trompette de Cavalerie", "FGIJ",    3, 0.6f,  0.0f,  false },
        { "Hautbois",               "FHIJ",    3, 0.6f,  0.02f, false },
        { "Cor de Chasse",          "EGK",     2, 0.6f,  0.0f,  false },
        { "Cor d'Harmonie",         "CK",      2, 0.6f,  0.0f,  false },
        { "Basson",                 "CGK",     1, 0.6f,  0.02f, false },
        { "Flute",                  "GJ",      3, 0.6f,  0.12f, false },
        { "Bugle",                  "CGJ",     2, 0.6f,  0.0f,  false },
        { "Petite Flute Pipeau",    "GI",      4, 0.6f,  0.10f, false },
        { "Mandoline",              "DFH",     3, 0.6f,  0.0f,  false },
        { "Banjo",                  "DFGIJ",   2, 0.6f,  0.0f,  false },
        { "Cornemuse",              "FG",      3, 0.6f,  0.10f, false },
        { "Orgue de Cinema",        "BCE2W",   2, 0.6f,  0.0f,  true  },
        { "Clarinette",             "BGI",     2, 0.6f,  0.02f, false },
        { "Bandoneon",              "A",       3, 0.6f,  0.0f,  false },
        { "Guitare Flamenco",       "FGHP",    2, 0.8f,  0.0f,  true  },
        { "Guitare Douce",          "CGIP",    2, 1.0f,  0.0f,  true  },
        { "Guitare Hawaienne",      "GIP2W",   2, 1.2f,  0.0f,  false },
        { "Clavecin",               "HP",      3, 0.7f,  0.0f,  true  },
        { "Cithare",                "FGIP2",   3, 1.0f,  0.0f,  true  },
        { "Castagnettes",           "FGP",     3, 0.12f, 0.15f, true  },
        { "Trombone",               "CJF",     2, 0.6f,  0.0f,  false },
        { "Contrebasse a Vent",     "BCE",     1, 0.6f,  0.0f,  false },
        { "Contrebasse a Corde",    "ABCEF",   1, 0.6f,  0.0f,  false },
    };

    inline constexpr int count = (int) std::size (list);
} // namespace ondaire::presets
