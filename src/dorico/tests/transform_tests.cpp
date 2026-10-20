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

#include <gtest/gtest.h>

#include "dorico/internal/transform.h"

using namespace mu::dorico;
using namespace mu::dorico::transform;
using mu::engraving::Fraction;
using mu::engraving::NoteVal;

namespace {
constexpr int C_MAJOR = 0;
constexpr int E_FLAT_MAJOR = -3;

Event note(int len, std::vector<int> pitches)
{
    Event e;
    e.len = Fraction(1, len);
    for (int p : pitches) {
        e.notes.push_back(NoteVal(p));
    }
    return e;
}

Event rest(int len)
{
    Event e;
    e.len = Fraction(1, len);
    return e;
}

std::vector<int> pitches(const std::vector<Event>& events)
{
    std::vector<int> out;
    for (const Event& e : events) {
        out.push_back(e.isRest() ? -1 : e.notes.front().pitch);
    }
    return out;
}

std::vector<Fraction> lengths(const std::vector<Event>& events)
{
    std::vector<Fraction> out;
    for (const Event& e : events) {
        out.push_back(e.len);
    }
    return out;
}

TransformSpec spec(TransformOp op, bool pt, bool rm, int steps = 1, bool invert = false)
{
    TransformSpec s;
    s.op = op;
    s.pitches = pt;
    s.rhythms = rm;
    s.steps = steps;
    s.invert = invert;
    return s;
}

// C D E F G in C major as quarter notes
std::vector<Event> scale()
{
    return { note(4, { 60 }), note(4, { 62 }), note(4, { 64 }), note(4, { 65 }), note(4, { 67 }) };
}
}

TEST(DoricoTransformTests, ReverseAndRotateOrder)
{
    EXPECT_EQ(reverseOrder(4), (std::vector<size_t> { 3, 2, 1, 0 }));
    EXPECT_EQ(reverseOrder(0), std::vector<size_t>());
    EXPECT_EQ(rotateOrder(4, 1), (std::vector<size_t> { 3, 0, 1, 2 })); // the last value moves to the front
    EXPECT_EQ(rotateOrder(4, -1), (std::vector<size_t> { 1, 2, 3, 0 }));
    EXPECT_EQ(rotateOrder(4, 0), (std::vector<size_t> { 0, 1, 2, 3 }));
    EXPECT_EQ(rotateOrder(4, 4), rotateOrder(4, 0));
    EXPECT_EQ(rotateOrder(4, 7), rotateOrder(4, 3));
    EXPECT_EQ(rotateOrder(4, -7), rotateOrder(4, -3));
    EXPECT_EQ(rotateOrder(4, -3), rotateOrder(4, 1));
}

TEST(DoricoTransformTests, ChromaticInversion)
{
    InvertContext ctx;
    ctx.diatonic = false;
    ctx.twiceCentre = 120; // around C4
    EXPECT_EQ(invertPitch(60, ctx), 60);
    EXPECT_EQ(invertPitch(62, ctx), 58);
    EXPECT_EQ(invertPitch(64, ctx), 56);
    ctx.twiceCentre = 2 * 63; // around E flat 4
    EXPECT_EQ(invertPitch(60, ctx), 66);
}

TEST(DoricoTransformTests, DiatonicInversionFollowsTheKey)
{
    int alter = 0;
    const int c4 = stepOf(60, C_MAJOR, alter);
    InvertContext ctx;
    ctx.key = C_MAJOR;
    ctx.stepSum = 2 * c4;
    EXPECT_EQ(invertPitch(60, ctx), 60);
    EXPECT_EQ(invertPitch(62, ctx), 59); // D -> B
    EXPECT_EQ(invertPitch(64, ctx), 57); // E -> A
    EXPECT_EQ(invertPitch(59, ctx), 62);

    // a minor second below becomes a major second above: E -> F is a semitone, E -> D a tone
    int a = 0;
    const int e4 = stepOf(64, C_MAJOR, a);
    ctx.stepSum = 2 * e4;
    EXPECT_EQ(invertPitch(65, ctx), 62); // F -> D
}

TEST(DoricoTransformTests, DiatonicInversionMirrorsAccidentals)
{
    int alter = 0;
    InvertContext ctx;
    ctx.key = C_MAJOR;
    ctx.stepSum = 2 * stepOf(60, C_MAJOR, alter);
    // D sharp is a semitone above D, so it lands a semitone below B: B flat
    EXPECT_EQ(invertPitch(63, ctx), 58);
}

TEST(DoricoTransformTests, DiatonicInversionInAFlatKey)
{
    int alter = 0;
    InvertContext ctx;
    ctx.key = E_FLAT_MAJOR;
    ctx.stepSum = 2 * stepOf(63, E_FLAT_MAJOR, alter); // around E flat 4
    EXPECT_EQ(invertPitch(63, ctx), 63);
    EXPECT_EQ(invertPitch(65, ctx), 62); // F -> D (a tone up becomes a tone down)
    EXPECT_EQ(invertPitch(67, ctx), 60); // G -> C
    EXPECT_EQ(invertPitch(68, ctx), 58); // A flat -> B flat
}

TEST(DoricoTransformTests, InversionOutOfRangeIsFlagged)
{
    InvertContext ctx;
    ctx.diatonic = false;
    ctx.twiceCentre = 240;
    EXPECT_EQ(invertPitch(120, ctx), 120);
    EXPECT_GT(invertPitch(20, ctx), 127);
}

TEST(DoricoTransformTests, ReversePitchesKeepsRhythmAndRests)
{
    std::vector<Event> in = { note(4, { 60 }), rest(8), note(8, { 62 }), note(2, { 64, 67 }) };
    const std::vector<Event> out = apply(in, spec(TransformOp::Reverse, true, false), InvertContext());
    EXPECT_EQ(pitches(out), (std::vector<int> { 64, -1, 62, 60 }));
    EXPECT_EQ(lengths(out), lengths(in));
    ASSERT_EQ(out[0].notes.size(), 2u); // the chord moves as a unit
    EXPECT_EQ(out[0].notes[1].pitch, 67);
}

TEST(DoricoTransformTests, ReverseRhythmsKeepsPitches)
{
    std::vector<Event> in = { note(4, { 60 }), note(8, { 62 }), rest(8), note(2, { 64 }) };
    const std::vector<Event> out = apply(in, spec(TransformOp::Reverse, false, true), InvertContext());
    EXPECT_EQ(pitches(out), pitches(in));
    EXPECT_EQ(lengths(out), (std::vector<Fraction> { Fraction(1, 2), Fraction(1, 8), Fraction(1, 8), Fraction(1, 4) }));
}

TEST(DoricoTransformTests, ReverseAllMovesWholeEvents)
{
    std::vector<Event> in = { note(4, { 60 }), rest(8), note(8, { 62 }), note(2, { 64 }) };
    const std::vector<Event> out = apply(in, spec(TransformOp::Reverse, true, true), InvertContext());
    EXPECT_EQ(pitches(out), (std::vector<int> { 64, 62, -1, 60 }));
    EXPECT_EQ(lengths(out), (std::vector<Fraction> { Fraction(1, 2), Fraction(1, 8), Fraction(1, 8), Fraction(1, 4) }));
}

TEST(DoricoTransformTests, RotatePitches)
{
    const std::vector<Event> in = scale();
    EXPECT_EQ(pitches(apply(in, spec(TransformOp::Rotate, true, false, 1), InvertContext())),
              (std::vector<int> { 67, 60, 62, 64, 65 }));
    EXPECT_EQ(pitches(apply(in, spec(TransformOp::Rotate, true, false, -2), InvertContext())),
              (std::vector<int> { 64, 65, 67, 60, 62 }));
    EXPECT_EQ(pitches(apply(in, spec(TransformOp::Rotate, true, false, 7), InvertContext())),
              pitches(apply(in, spec(TransformOp::Rotate, true, false, 2), InvertContext())));
}

TEST(DoricoTransformTests, RotateRhythms)
{
    const std::vector<Event> in = { note(4, { 60 }), note(8, { 62 }), note(8, { 64 }), note(2, { 65 }) };
    const std::vector<Event> out = apply(in, spec(TransformOp::Rotate, false, true, -3), InvertContext());
    EXPECT_EQ(pitches(out), pitches(in));
    EXPECT_EQ(lengths(out), (std::vector<Fraction> { Fraction(1, 2), Fraction(1, 4), Fraction(1, 8), Fraction(1, 8) }));
}

TEST(DoricoTransformTests, InvertAllNotesOfAChord)
{
    InvertContext ctx;
    ctx.diatonic = false;
    ctx.twiceCentre = 2 * 60;
    TransformSpec s;
    s.invert = true;
    const std::vector<Event> in = { note(4, { 60, 64, 67 }) };
    const std::vector<Event> out = apply(in, s, ctx);
    ASSERT_EQ(out[0].notes.size(), 3u);
    EXPECT_EQ(out[0].notes[0].pitch, 53); // sorted ascending: 60-7, 60-4, 60
    EXPECT_EQ(out[0].notes[1].pitch, 56);
    EXPECT_EQ(out[0].notes[2].pitch, 60);
    EXPECT_EQ(out[0].notes[0].tpc1, mu::engraving::Tpc::TPC_INVALID);
}

TEST(DoricoTransformTests, ReverseAndInvertOrdersThenMirrors)
{
    InvertContext ctx;
    ctx.diatonic = false;
    ctx.twiceCentre = 2 * 60;
    const std::vector<Event> out = apply(scale(), spec(TransformOp::Reverse, true, false, 1, true), ctx);
    EXPECT_EQ(pitches(out), (std::vector<int> { 53, 55, 56, 58, 60 })); // G F E D C reversed, then 120 - p
}
