/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "keysigparser.h"

#include <cctype>
#include <cstring>

using namespace mu::dorico;

namespace {
const char* const TONICS = "FCGDAEB"; // tonics along the circle of fifths: F = -1, C = 0, ... B = 5
const int MAX_FIFTHS = 7;

std::string trimSpaces(const std::string& s)
{
    const size_t b = s.find_first_not_of(' ');
    if (b == std::string::npos) {
        return std::string();
    }
    return s.substr(b, s.find_last_not_of(' ') - b + 1);
}

std::string lowerCase(std::string s)
{
    for (char& c : s) {
        c = char(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

bool isPrefixOf(const std::string& s, const std::string& word)
{
    return word.compare(0, s.size(), s) == 0;
}

int accidental(char c) // +1 sharp, -1 flat, 0 none
{
    switch (c) {
    case '#': case 's': return 1;
    case 'b': case 'f': return -1;
    default: return 0;
    }
}
}

KeySigParse mu::dorico::parseKeySig(const std::string& input)
{
    const std::string text = trimSpaces(input);
    if (text.empty()) {
        return { ParseStatus::Incomplete, {} };
    }

    KeySigParse result;
    const std::string word = lowerCase(text);
    if (word == "open" || word == "atonal") {
        result.status = ParseStatus::Ok;
        result.entry.kind = KeySigEntry::Kind::Open;
        return result;
    }

    // 3#, 4f: a major key by its number of sharps or flats.
    if (std::isdigit(static_cast<unsigned char>(text[0]))) {
        const int n = text[0] - '0';
        if (n < 1 || n > MAX_FIFTHS || text.size() > 2) {
            return result;
        }
        if (text.size() == 1) {
            result.status = ParseStatus::Incomplete;
            return result;
        }
        const int acc = accidental(text[1]);
        if (acc != 0) {
            result.status = ParseStatus::Ok;
            result.entry.fifths = acc * n;
        }
        return result;
    }

    // Eb, f#: upper case is major, lower case minor. A minor tonic sits 3 fifths above its relative major's.
    const char* letter = std::strchr(TONICS, std::toupper(static_cast<unsigned char>(text[0])));
    if (letter && *letter && text.size() <= 2) {
        const int acc = text.size() == 2 ? accidental(text[1]) : 0;
        const bool minor = std::islower(static_cast<unsigned char>(text[0]));
        const int fifths = int(letter - TONICS) - 1 + 7 * acc - (minor ? 3 : 0);
        if ((text.size() == 1 || acc != 0) && fifths >= -MAX_FIFTHS && fifths <= MAX_FIFTHS) {
            result.status = ParseStatus::Ok;
            result.entry.fifths = fifths;
            result.entry.minor = minor;
            return result;
        }
        // "D#" is out of range; "at" is not a key but is on its way to "atonal".
    }

    if (isPrefixOf(word, "open") || isPrefixOf(word, "atonal")) {
        result.status = ParseStatus::Incomplete;
    }
    return result;
}

std::string mu::dorico::keySigText(const KeySigEntry& entry)
{
    if (entry.kind == KeySigEntry::Kind::Open) {
        return "open";
    }
    const int tonic = entry.fifths + (entry.minor ? 3 : 0) + 1; // index into TONICS, before wrapping
    const int acc = tonic >= 7 ? 1 : tonic < 0 ? -1 : 0;
    std::string text(1, TONICS[tonic - 7 * acc]);
    if (entry.minor) {
        text[0] = char(std::tolower(static_cast<unsigned char>(text[0])));
    }
    if (acc) {
        text += acc > 0 ? '#' : 'b';
    }
    return text;
}
