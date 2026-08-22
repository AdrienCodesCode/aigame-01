#include "game/gameplay_scenario.hpp"

#include <array>

namespace wide_eye::game {
namespace {

struct NamedGameplayScenario {
    std::string_view name;
    GameplayScenarioDefinition definition;
};

constexpr SheepStateBuffer kSeparationSheepStates{{
    {.id = 1,
     .position = {.x = 15.25, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 2,
     .position = {.x = 15.25, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 18.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 20.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 5,
     .position = {.x = 22.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
}};

constexpr SheepStateBuffer kAttractionSheepStates{{
    {.id = 1,
     .position = {.x = 15.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 2,
     .position = {.x = 15.5, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 15.0, .y = 1.0, .z = 20.5},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 14.25, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 5,
     .position = {.x = 15.0, .y = 1.0, .z = 19.0},
     .heading_radians = 0.0,
     .grounded = true},
}};

constexpr SheepStateBuffer kAlignmentSheepStates{{
    {.id = 1,
     .position = {.x = 15.0, .y = 1.0, .z = 20.0},
     .velocity = {.x = 1.0},
     .heading_radians = 1.57079632679489661923,
     .grounded = true},
    {.id = 2,
     .position = {.x = 15.5, .y = 1.0, .z = 20.0},
     .velocity = {.z = -1.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 18.0, .y = 1.0, .z = 20.0},
     .velocity = {.z = -1.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 20.0, .y = 1.0, .z = 20.0},
     .velocity = {.z = -1.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 5,
     .position = {.x = 22.0, .y = 1.0, .z = 20.0},
     .velocity = {.z = -1.0},
     .heading_radians = 0.0,
     .grounded = true},
}};

constexpr SheepStateBuffer kDogPressureSheepStates{{
    {.id = 1,
     .position = {.x = 14.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 2,
     .position = {.x = 15.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 18.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 19.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 5,
     .position = {.x = 12.0, .y = 1.0, .z = 24.0},
     .heading_radians = 0.0,
     .grounded = true},
}};

// One moving dog at (12, 20) travelling +x at 4.0 world units/s isolates approach
// velocity: sheep 1 closes head-on above the saturation speed, sheep 2 sits
// exactly abeam, sheep 3 is behind the dog and opening, sheep 4 closes on an
// exact 3-4-5 diagonal, and sheep 5 closes from outside the pressure radius.
constexpr SheepStateBuffer kDogApproachSheepStates{{
    {.id = 1,
     .position = {.x = 14.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 2,
     .position = {.x = 12.0, .y = 1.0, .z = 23.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 9.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 15.0, .y = 1.0, .z = 24.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 5,
     .position = {.x = 20.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
}};

constexpr DogState kApproachDogState{.position = {.x = 12.0, .y = 1.0, .z = 20.0},
                                     .velocity = {.x = 4.0},
                                     .heading_radians = 1.57079632679489661923,
                                     .grounded = true};

// One stationary dog at (12, 20) isolates facing from approach velocity: heading
// zero is exactly the -z forward direction, so sheep 1 is straight ahead, sheep 2
// is exactly abeam, sheep 3 is directly behind, sheep 4 sits on an exact 3-4-5
// diagonal ahead of the dog, and sheep 5 is straight ahead but outside the
// pressure radius.
//
// Sheep 4's `(15, 16)` is deliberately left standing on the closed gate's north
// face, and this pair is the only named fixture that starts a body inside an
// obstacle. It is kept because it is the QA-001 witness: with the defect
// present that sheep walked through the closed gate, so any return of it changes
// these two scenarios by name. It cannot be moved without also moving the
// accepted first-tick `0.8` facing cosine it carries — every 3-4-5 offset from
// this dog that produces that cosine lies on the paddock's own wall line.
constexpr SheepStateBuffer kDogFacingSheepStates{{
    {.id = 1,
     .position = {.x = 12.0, .y = 1.0, .z = 18.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 2,
     .position = {.x = 15.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 12.0, .y = 1.0, .z = 23.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 15.0, .y = 1.0, .z = 16.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 5,
     .position = {.x = 12.0, .y = 1.0, .z = 12.0},
     .heading_radians = 0.0,
     .grounded = true},
}};

constexpr DogState kFacingDogState{
    .position = {.x = 12.0, .y = 1.0, .z = 20.0}, .heading_radians = 0.0, .grounded = true};

// One stationary dog at (16, 13) just north of the paddock wall line isolates
// line of sight against the analytic obstacles the dog itself collides with:
// sheep 1 shares the open ground north of the walls on a clear line, sheep 2 and
// sheep 4 stand south of the left and right walls at an exact 3-4-5 distance of
// 5, sheep 3 stands south of the gate opening and sees the dog only while that
// gate is open, and sheep 5 is hidden by the left wall from outside the pressure
// radius. The dog heading looks south down the gate line so a derived fixture
// can observe the approach and facing terms releasing together with pressure.
constexpr SheepStateBuffer kDogLineOfSightSheepStates{{
    {.id = 1,
     .position = {.x = 16.0, .y = 1.0, .z = 9.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 2,
     .position = {.x = 13.0, .y = 1.0, .z = 17.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 16.0, .y = 1.0, .z = 18.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 19.0, .y = 1.0, .z = 17.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 5,
     .position = {.x = 10.0, .y = 1.0, .z = 21.0},
     .heading_radians = 0.0,
     .grounded = true},
}};

constexpr DogState kLineOfSightDogState{.position = {.x = 16.0, .y = 1.0, .z = 13.0},
                                        .heading_radians = 3.14159265358979323846,
                                        .grounded = true};

// One stationary dog parked well clear of the wall line and five sheep given an
// exact initial velocity isolate paddock collision from every steering term: no
// social or dog term is enabled, so each sheep travels in a straight line at a
// constant speed until the analytic paddock stops it, which makes every resting
// coordinate exact arithmetic. Every sheep that contacts something starts an exact
// 3.5 units of travel along its blocked axis from the limit that stops it.
// Sheep 1 runs head-on into the left wall, sheep 2 runs
// at the gate line and is the paired variable, sheep 3 arrives diagonally at the
// right wall so one axis is blocked while the other keeps running, sheep 4 never
// touches anything and is the untouched control, and sheep 5 runs at the
// paddock's own outer bound, which stops a sheep without being one of the named
// obstacle shapes.
constexpr double kPaddockCollisionSheepSpeed = 3.0;

constexpr SheepStateBuffer kPaddockCollisionSheepStates{{
    {.id = 1,
     .position = {.x = 8.0, .y = 1.0, .z = 20.0},
     .velocity = {.z = -kPaddockCollisionSheepSpeed},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 2,
     .position = {.x = 16.0, .y = 1.0, .z = 20.0},
     .velocity = {.z = -kPaddockCollisionSheepSpeed},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 24.0, .y = 1.0, .z = 20.0},
     .velocity = {.x = -kPaddockCollisionSheepSpeed, .z = -kPaddockCollisionSheepSpeed},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 28.0, .y = 1.0, .z = 26.0},
     .velocity = {.x = -kPaddockCollisionSheepSpeed},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 5,
     .position = {.x = 4.0, .y = 1.0, .z = 20.0},
     .velocity = {.x = -kPaddockCollisionSheepSpeed},
     .heading_radians = 0.0,
     .grounded = true},
}};

constexpr DogState kPaddockCollisionDogState{
    .position = {.x = 16.0, .y = 1.0, .z = 28.0}, .heading_radians = 0.0, .grounded = true};

// One stationary dog at (16, 26) with five sheep on an exact 5-unit ring around
// it isolates temperament: every sheep sees the same prior-state distance, the
// same falloff, and the same dog, so the only thing that can make two of them
// move differently is the temperament each one carries. Each offset is an exact
// 3-4-5 or 0-5-5 triangle, and the dog heading of zero looks straight down the
// -z axis at the ring so a derived fixture can enable the approach and facing
// terms without moving anything. The ring sits five units north of the wall
// line, so no sheep starts within its own body radius of an obstacle face. The
// nervous and stubborn sheep are placed in mirrored pairs across the gate line,
// so a per-temperament result cannot be an artifact of one bearing.
constexpr SheepStateBuffer kTemperamentSheepStates{{
    {.id = 1,
     .position = {.x = 16.0, .y = 1.0, .z = 21.0},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::ordinary,
     .grounded = true},
    {.id = 2,
     .position = {.x = 13.0, .y = 1.0, .z = 22.0},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::nervous,
     .grounded = true},
    {.id = 3,
     .position = {.x = 19.0, .y = 1.0, .z = 22.0},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::stubborn,
     .grounded = true},
    {.id = 4,
     .position = {.x = 12.0, .y = 1.0, .z = 23.0},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::stubborn,
     .grounded = true},
    {.id = 5,
     .position = {.x = 20.0, .y = 1.0, .z = 23.0},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::nervous,
     .grounded = true},
}};

constexpr DogState kTemperamentDogState{
    .position = {.x = 16.0, .y = 1.0, .z = 26.0}, .heading_radians = 0.0, .grounded = true};

// One dog at (16, 20) closing south at exactly the approach reference speed,
// looking straight down the same axis, isolates the combined-influence bound.
// Everything sits at least four units south of the paddock wall line, so no
// sheep starts within its own body radius of an obstacle face and collision
// cannot contaminate a steering measurement.
//
// Sheep 1 is the over-bound subject: it stands an exact 3 units south of the dog,
// so the shared 6-unit falloff is exactly 0.5, and it carries the nervous
// temperament, so its pressure, approach, and facing responses double to 3.0,
// 2.0, and 1.5. Sheep 2 stands 0.625 north of it, which is inside the 1-unit
// separation radius, so sheep 1 is also pushed south by exactly 1.5 of
// separation. Its six terms therefore sum to exactly 8.0 straight down +z:
// twice the 4.0 bound, so the applied scale is exactly 0.5 and the bounded
// result is exactly the bound. Sheep 5 is a second over-bound subject on an
// exact 3-4-5 diagonal at distance 3.75, so the bound is observed on a direction
// that is not axis-aligned. Sheep 3 is the under-bound control: 4.5 units south
// of the dog, ordinary, and more than a separation radius from anyone, so its
// three dog terms sum to exactly 1.625 and the bound must leave it alone. Sheep 4
// stands outside the pressure radius with no neighbour at all, so its sum is
// exactly zero and it observes that an untouched sheep still publishes a scale of
// one.
constexpr SheepStateBuffer kCombinedInfluenceSheepStates{{
    {.id = 1,
     .position = {.x = 16.0, .y = 1.0, .z = 23.0},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::nervous,
     .grounded = true},
    {.id = 2,
     .position = {.x = 16.0, .y = 1.0, .z = 22.375},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::ordinary,
     .grounded = true},
    {.id = 3,
     .position = {.x = 16.0, .y = 1.0, .z = 24.5},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::ordinary,
     .grounded = true},
    {.id = 4,
     .position = {.x = 12.0, .y = 1.0, .z = 28.0},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::stubborn,
     .grounded = true},
    {.id = 5,
     .position = {.x = 18.25, .y = 1.0, .z = 23.0},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::nervous,
     .grounded = true},
}};

constexpr DogState kCombinedInfluenceDogState{.position = {.x = 16.0, .y = 1.0, .z = 20.0},
                                              .velocity = {.z = 3.0},
                                              .heading_radians = 3.14159265358979323846,
                                              .grounded = true};

// One stationary dog at (16, 30) and five sheep given exact initial velocities
// isolate the speed clamp and the turn rate from every steering term: no social
// or dog term is enabled, so nothing changes a sheep's velocity except the
// clamp, and every published number is exact arithmetic on the fixture's own
// values. Everything starts at least one unit clear of the wall line and far
// enough from the paddock's outer bounds that neither member contacts anything
// during the measured window. The dog is present only so the fixture still
// publishes prior-state bearing evidence against a heading that now changes.
//
// Sheep 1 travels straight down +z at exactly 12.5 world units/s — two and a
// half times the 5.0 maximum — so the clamp's scale is exactly 0.4 and the
// clamped speed is exactly the maximum. It already faces its motion, so it also
// observes that an aligned sheep is not rotated at all. Sheep 2 travels on an
// exact 3-4-5 diagonal at the same 12.5, so the clamp is observed on a direction
// that is not axis-aligned and lands on exactly (3, 4). Sheep 3 is the
// under-limit control at 2.0 and already faces its motion, so the limits must
// leave it bit-identical to the off member. Sheep 4 stands still with a heading
// of 1 radian, so it observes that a sheep with no motion keeps the way it was
// facing instead of snapping to face north. Sheep 5 travels at 3.0 — under the
// speed maximum, so the turn rate is the only limit acting on it — in exactly
// the opposite direction to its heading, which is the slowest turn the rule can
// be asked for and takes many ticks to complete.
constexpr SheepStateBuffer kMotionLimitSheepStates{{
    {.id = 1,
     .position = {.x = 8.0, .y = 1.0, .z = 18.0},
     .velocity = {.z = 12.5},
     .heading_radians = 3.14159265358979323846,
     .grounded = true},
    {.id = 2,
     .position = {.x = 22.0, .y = 1.0, .z = 18.0},
     .velocity = {.x = 7.5, .z = 10.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 16.0, .y = 1.0, .z = 26.0},
     .velocity = {.z = -2.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 12.0, .y = 1.0, .z = 28.0},
     .heading_radians = 1.0,
     .grounded = true},
    {.id = 5,
     .position = {.x = 20.0, .y = 1.0, .z = 26.0},
     .velocity = {.z = 3.0},
     .heading_radians = 0.0,
     .grounded = true},
}};

constexpr DogState kMotionLimitDogState{
    .position = {.x = 16.0, .y = 1.0, .z = 30.0}, .heading_radians = 0.0, .grounded = true};

// One stationary dog parked clear of everything and five sheep given exact
// initial velocities isolate obstacle and drop avoidance from every steering
// term: nothing else can change a sheep's straight-line motion, so every
// first-tick number is exact arithmetic on the fixture's own values and the
// paired control is today's uncorrected behavior. The `6.25` look-ahead and the
// `4.0` maximum are the accepted configuration, and every sheep below starts an
// exact `3.125` — half the look-ahead — from the face it is heading at, so the
// linear falloff gives an urgency of exactly `0.5` and a magnitude of exactly
// `2.0`. Half is chosen because it is the fraction that stays exact: a ratio
// like four fifths is not a binary fraction, and the oracle would have to fall
// back to a tolerance for no reason.
//
// Sheep 1 runs head-on at the middle of the left wall. Its nearer free edge is
// `6.5` away — further than it can see — so no way round is within reach and it
// gets the pure away-from-the-face push of exactly `(0, 2)`. Sheep 2 runs at
// the same wall `1.5` from its `+x` end, which is a way round it can reach, so
// its push is the same magnitude turned exactly halfway toward that end: the two
// components are exactly equal and it is deflected sideways as well as slowed.
// Sheep 3 runs head-on at the exact middle of the closed gate, where the two
// free edges are exactly equidistant and neither is nearer, so it observes that
// an exact tie keeps the pure push rather than inventing a side. Sheep 4 runs at
// the paddock's own outer bound, the only drop that exists while the ground is
// flat, and gets the full `4.0` straight back along its approach. Sheep 5 runs
// parallel to the wall line and only `3.5` from it: closer than the look-ahead,
// but with nothing in front of it, so it observes that the term reads direction
// rather than proximity and leaves an untouched sheep bit-identical to the
// control.
constexpr double kAvoidanceSheepSpeed = 3.0;

constexpr SheepStateBuffer kAvoidanceSheepStates{{
    {.id = 1,
     .position = {.x = 7.0, .y = 1.0, .z = 19.625},
     .velocity = {.z = -kAvoidanceSheepSpeed},
     .heading_radians = 3.14159265358979323846,
     .grounded = true},
    {.id = 2,
     .position = {.x = 13.0, .y = 1.0, .z = 19.625},
     .velocity = {.z = -kAvoidanceSheepSpeed},
     .heading_radians = 3.14159265358979323846,
     .grounded = true},
    {.id = 3,
     .position = {.x = 16.0, .y = 1.0, .z = 19.625},
     .velocity = {.z = -kAvoidanceSheepSpeed},
     .heading_radians = 3.14159265358979323846,
     .grounded = true},
    {.id = 4,
     .position = {.x = 4.0, .y = 1.0, .z = 26.0},
     .velocity = {.x = -kAvoidanceSheepSpeed},
     .heading_radians = -1.57079632679489661923,
     .grounded = true},
    {.id = 5,
     .position = {.x = 8.0, .y = 1.0, .z = 20.0},
     .velocity = {.x = kAvoidanceSheepSpeed},
     .heading_radians = 1.57079632679489661923,
     .grounded = true},
}};

constexpr DogState kAvoidanceDogState{
    .position = {.x = 26.0, .y = 1.0, .z = 26.0}, .heading_radians = 0.0, .grounded = true};

// One stationary dog and five sheep isolate the arousal proxy and the four
// behavior transitions. No steering term is enabled, so nothing can accelerate a
// sheep at all and the paired control is bit-identical in every field except the
// two the transitions write. Temperament *is* enabled: it adds no vector of its
// own with every dog term switched off, so its only effect here is the response
// scale it applies to the arousal stimulus, which is exactly the variable two of
// these sheep exist to show.
//
// The fixture widens the shared dog-pressure radius to exactly `8.0`. The rule
// is unchanged — arousal reads the same linear distance falloff the dog terms
// read — but at that radius one tick of the subject's exact `3.75` world-units/s
// travel changes the stimulus by exactly `1/128`, so the whole stimulus curve
// and every arousal value on it are exactly representable and the oracle can pin
// them with equality.
//
// Sheep 1 is the cycle subject: it is given an exact constant velocity that
// carries it from exactly the stimulus radius, straight through the dog, and out
// to the far side again, so the dog-to-sheep distance closes and opens exactly
// as it would if the dog walked in and left. Moving the sheep rather than the
// dog is what keeps the arithmetic exact — the stimulus depends only on the
// relative geometry, and the dog motor's acceleration is not a binary fraction —
// and the focused oracle additionally drives the *dog* through the same
// approach/hold/release to show the sequence does not depend on which body
// moves. Sheep 2 stands at exactly the stimulus radius, where the falloff is
// exactly zero, so it is the in-fixture control that must stay `settled` with an
// arousal of exactly `0`. Sheep 3 stands where the stimulus is exactly `0.625`,
// inside the driven hysteresis band, so a derived run that starts it already
// `driven` holds `driven` at the same arousal this one holds `alert` at. Sheep 4
// is nervous and sheep 5 is stubborn at the same exact `5`-unit distance, so one
// sits exactly on the driven entry threshold and the other never becomes alert
// at all from an identical cause.
constexpr double kBehaviorStimulusRadius = 8.0;
constexpr double kBehaviorSubjectSpeed = 3.75;

constexpr SheepStateBuffer kBehaviorTransitionSheepStates{{
    {.id = 1,
     .position = {.x = 30.0, .y = 1.0, .z = 20.0},
     .velocity = {.x = -kBehaviorSubjectSpeed},
     .heading_radians = -1.57079632679489661923,
     .grounded = true},
    {.id = 2,
     .position = {.x = 22.0, .y = 1.0, .z = 28.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 22.0, .y = 1.0, .z = 23.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 25.0, .y = 1.0, .z = 24.0},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::nervous,
     .grounded = true},
    {.id = 5,
     .position = {.x = 19.0, .y = 1.0, .z = 24.0},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::stubborn,
     .grounded = true},
}};

constexpr DogState kBehaviorTransitionDogState{
    .position = {.x = 22.0, .y = 1.0, .z = 20.0}, .heading_radians = 0.0, .grounded = true};

// The one deliberately maximal fixture: every steering term, the combined bound,
// both motion limits, temperament, line of sight, and the behavior transitions
// switched on together against a dog that is actually driving the flock.
//
// **This is a diagnostic, not accepted tuned gameplay.** Every other sheep
// fixture isolates one variable so its arithmetic can be pinned exactly; this one
// exists for the opposite reason — the properties that only appear when
// everything runs at once. It answers "is the applied acceleration always exactly
// the published terms", "does any term flip sign every tick", and "is the whole
// published sequence reproducible", none of which an isolated fixture can be
// asked. Nothing here is a claim that these five sheep move well, that these
// starting positions are a designed encounter, or that the accepted magnitudes
// have been tuned against each other; no owner has reviewed this motion.
//
// The geometry is chosen so that all seven steering terms have something to do
// from the first tick rather than only after the flock has drifted. Sheep 1 and
// sheep 2 start `0.901…` apart, inside the `1.0` separation radius; every pair is
// inside the `4.0` attraction radius and every sheep has a neighbour inside the
// `3.0` alignment radius; every sheep starts with a different velocity, so
// alignment has a disagreement to resolve and avoidance has a direction to probe;
// the dog stands between `3.04` and `5.0` south of them, inside the `6.0`
// pressure radius and looking north straight at the flock, so distance,
// approach, and facing all respond; and every sheep is moving north toward the
// closed wall line, so avoidance meets a face inside its `6.25` look-ahead as
// the dog presses the flock into it. Two
// sheep are `nervous` and one is `stubborn`, so the published temperament scale
// varies within one run.
//
// Every sheep starts at least `0.5` — one sheep body radius — clear of every
// obstacle face, which QA-001 requires until the analytic field stops bodies that
// begin inside their own radius band. The gate is closed, so the wall line is a
// continuous barrier the flock is pressed against and released from; the dog's
// sight line is therefore never occluded here, and the paired
// `sheep-dog-line-of-sight-off`/`-on` fixtures remain the evidence for that rule.
constexpr SheepStateBuffer kAllInfluencesSheepStates{{
    {.id = 1,
     .position = {.x = 16.0, .y = 1.0, .z = 22.0},
     .velocity = {.z = -1.0},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::ordinary,
     .grounded = true},
    {.id = 2,
     .position = {.x = 16.75, .y = 1.0, .z = 22.5},
     .velocity = {.x = 0.5, .z = -1.0},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::nervous,
     .grounded = true},
    {.id = 3,
     .position = {.x = 14.5, .y = 1.0, .z = 22.75},
     .velocity = {.x = -0.75, .z = -0.25},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::stubborn,
     .grounded = true},
    {.id = 4,
     .position = {.x = 18.0, .y = 1.0, .z = 23.5},
     .velocity = {.z = -0.5},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::ordinary,
     .grounded = true},
    {.id = 5,
     .position = {.x = 15.5, .y = 1.0, .z = 24.0},
     .velocity = {.x = 0.25, .z = -1.25},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::nervous,
     .grounded = true},
}};

constexpr DogState kAllInfluencesDogState{
    .position = {.x = 16.0, .y = 1.0, .z = 27.0}, .heading_radians = 0.0, .grounded = true};

// Fifty authoritative sheep, placed by an explicit rule rather than by a
// generator: this simulation contains no randomness and no rule reads the seed,
// so a fixture that needed one would be describing a game this is not.
//
// The rule is a row-major lattice. Member `i` stands at column `i % 10` and row
// `i / 10` of a 10 x 5 grid whose spacing is 2.5 world units, anchored so the
// grid is centred on the paddock's x axis.
//
// The spacing is chosen, not tuned. It is above the 1.0 body diameter, so no
// member starts inside another; above the 1.0 close-range separation radius, so
// the flock starts settled rather than already pushing itself apart; and above
// the 2.2-unit length of the renderer's sheep proxy, so the drawn bodies do not
// interpenetrate either. It is below the 4.0 attraction radius, so the flock is
// a flock rather than fifty independent animals.
//
// Every constant is checked against the analytic paddock rather than asserted:
// the occupied rectangle is x in [4.75, 27.25] and z in [18, 28], which with the
// 0.5 sheep body radius reaches x in [4.25, 27.75] and z in [17.5, 28.5]. That
// clears the paddock bounds at 0 and 32 on both axes, and clears the wall line
// -- both walls and the gate end at z = 16 -- by one and a half units.
// `resolve_sheep_against_paddock` would depenetrate a bad start, but a fixture
// that relies on that is a fixture that has not been placed.
inline constexpr std::size_t kFiftySheepCount = 50;
inline constexpr std::size_t kFiftySheepColumns = 10;
inline constexpr double kFiftySheepSpacing = 2.5;
inline constexpr double kFiftySheepOriginX = 4.75;
inline constexpr double kFiftySheepOriginZ = 18.0;

[[nodiscard]] constexpr SheepStateBuffer make_fifty_sheep_states() noexcept {
    SheepStateBuffer sheep{};
    for (std::size_t index = 0; index < kFiftySheepCount; ++index) {
        const auto column = static_cast<double>(index % kFiftySheepColumns);
        const auto row = static_cast<double>(index / kFiftySheepColumns);
        sheep[index] = {
            .id = static_cast<std::uint32_t>(index + 1),
            .position = {.x = kFiftySheepOriginX + kFiftySheepSpacing * column,
                         .y = PaddockCollisionField::kGroundHeight,
                         .z = kFiftySheepOriginZ + kFiftySheepSpacing * row},
            .heading_radians = 0.0,
            .grounded = true,
        };
    }
    return sheep;
}

constexpr SheepStateBuffer kFiftySheepStates = make_fifty_sheep_states();

// Behind the lattice and clear of it, facing up the paddock at the flock, so the
// dog terms have something to act on from the first tick. Heading zero is the
// `-z` direction in the shared `atan2(x, -z)` convention, which is toward the
// wall line the flock stands in front of. At the 0.42 dog radius this start
// clears the paddock bound at 32 and stands 2.5 units behind the rear row.
constexpr DogState kFiftySheepDogState{
    .position = {.x = 16.0, .y = 1.0, .z = 30.5}, .heading_radians = 0.0, .grounded = true};

constexpr std::array<NamedGameplayScenario, 31> kGameplayScenarios{{
    {
        .name = "paddock-start",
        .definition = {.id = GameplayScenarioId::paddock_start,
                       .dog = {.initial_state = {.position = {.x = 16.0, .y = 1.0, .z = 24.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}}},
    },
    {
        .name = "presentation-motion",
        .definition = {.id = GameplayScenarioId::presentation_motion,
                       .dog = {.initial_state = {.position = {.x = 16.0, .y = 1.0, .z = 24.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}},
                       .sheep_fixture = SheepFixture::scripted_presentation_motion},
    },
    {
        .name = "wall-contact",
        .definition = {.id = GameplayScenarioId::wall_contact,
                       .dog = {.initial_state = {.position = {.x = 8.0, .y = 1.0, .z = 20.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}}},
    },
    {
        .name = "closed-gate",
        .definition = {.id = GameplayScenarioId::closed_gate,
                       .dog = {.initial_state = {.position = {.x = 16.0, .y = 1.0, .z = 20.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}}},
    },
    {
        .name = "open-gate",
        .definition = {.id = GameplayScenarioId::open_gate,
                       .dog = {.initial_state = {.position = {.x = 16.0, .y = 1.0, .z = 20.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}},
                       .gate_open = true},
    },
    {
        .name = "sheep-only-separation",
        .definition = {.id = GameplayScenarioId::sheep_only_separation,
                       .dog = {.initial_state = {.position = {.x = 16.0, .y = 1.0, .z = 24.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kSeparationSheepStates,
                       .sheep_separation = {.enabled = true}},
    },
    {
        .name = "sheep-only-attraction",
        .definition = {.id = GameplayScenarioId::sheep_only_attraction,
                       .dog = {.initial_state = {.position = {.x = 16.0, .y = 1.0, .z = 24.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kAttractionSheepStates,
                       .sheep_attraction = {.enabled = true}},
    },
    {
        .name = "sheep-alignment-off",
        .definition = {.id = GameplayScenarioId::sheep_alignment_off,
                       .dog = {.initial_state = {.position = {.x = 16.0, .y = 1.0, .z = 24.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kAlignmentSheepStates},
    },
    {
        .name = "sheep-alignment-on",
        .definition = {.id = GameplayScenarioId::sheep_alignment_on,
                       .dog = {.initial_state = {.position = {.x = 16.0, .y = 1.0, .z = 24.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kAlignmentSheepStates,
                       .sheep_alignment = {.enabled = true}},
    },
    {
        .name = "sheep-dog-pressure-off",
        .definition = {.id = GameplayScenarioId::sheep_dog_pressure_off,
                       .dog = {.initial_state = {.position = {.x = 12.0, .y = 1.0, .z = 20.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kDogPressureSheepStates},
    },
    {
        .name = "sheep-dog-pressure-on",
        .definition = {.id = GameplayScenarioId::sheep_dog_pressure_on,
                       .dog = {.initial_state = {.position = {.x = 12.0, .y = 1.0, .z = 20.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kDogPressureSheepStates,
                       .sheep_dog_pressure = {.enabled = true}},
    },
    {
        .name = "sheep-dog-approach-off",
        .definition = {.id = GameplayScenarioId::sheep_dog_approach_off,
                       .dog = {.initial_state = kApproachDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kDogApproachSheepStates,
                       .sheep_dog_pressure = {.enabled = true}},
    },
    {
        .name = "sheep-dog-approach-on",
        .definition = {.id = GameplayScenarioId::sheep_dog_approach_on,
                       .dog = {.initial_state = kApproachDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kDogApproachSheepStates,
                       .sheep_dog_pressure = {.enabled = true},
                       .sheep_dog_approach = {.enabled = true}},
    },
    {
        .name = "sheep-dog-facing-off",
        .definition = {.id = GameplayScenarioId::sheep_dog_facing_off,
                       .dog = {.initial_state = kFacingDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kDogFacingSheepStates,
                       .sheep_dog_pressure = {.enabled = true}},
    },
    {
        .name = "sheep-dog-facing-on",
        .definition = {.id = GameplayScenarioId::sheep_dog_facing_on,
                       .dog = {.initial_state = kFacingDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kDogFacingSheepStates,
                       .sheep_dog_pressure = {.enabled = true},
                       .sheep_dog_facing = {.enabled = true}},
    },
    {
        .name = "sheep-dog-line-of-sight-off",
        .definition = {.id = GameplayScenarioId::sheep_dog_line_of_sight_off,
                       .dog = {.initial_state = kLineOfSightDogState},
                       .gate_open = true,
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kDogLineOfSightSheepStates,
                       .sheep_dog_pressure = {.enabled = true}},
    },
    {
        .name = "sheep-dog-line-of-sight-on",
        .definition = {.id = GameplayScenarioId::sheep_dog_line_of_sight_on,
                       .dog = {.initial_state = kLineOfSightDogState},
                       .gate_open = true,
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kDogLineOfSightSheepStates,
                       .sheep_dog_pressure = {.enabled = true},
                       .sheep_dog_line_of_sight = {.enabled = true}},
    },
    {
        .name = "sheep-paddock-collision-closed-gate",
        .definition = {.id = GameplayScenarioId::sheep_paddock_collision_closed_gate,
                       .dog = {.initial_state = kPaddockCollisionDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kPaddockCollisionSheepStates},
    },
    {
        .name = "sheep-paddock-collision-open-gate",
        .definition = {.id = GameplayScenarioId::sheep_paddock_collision_open_gate,
                       .dog = {.initial_state = kPaddockCollisionDogState},
                       .gate_open = true,
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kPaddockCollisionSheepStates},
    },
    {
        .name = "sheep-temperament-neutral",
        .definition = {.id = GameplayScenarioId::sheep_temperament_neutral,
                       .dog = {.initial_state = kTemperamentDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kTemperamentSheepStates,
                       .sheep_dog_pressure = {.enabled = true}},
    },
    {
        .name = "sheep-temperament-varied",
        .definition = {.id = GameplayScenarioId::sheep_temperament_varied,
                       .dog = {.initial_state = kTemperamentDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kTemperamentSheepStates,
                       .sheep_dog_pressure = {.enabled = true},
                       .sheep_temperament = {.enabled = true}},
    },
    {
        .name = "sheep-combined-influence-off",
        .definition = {.id = GameplayScenarioId::sheep_combined_influence_off,
                       .dog = {.initial_state = kCombinedInfluenceDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kCombinedInfluenceSheepStates,
                       .sheep_separation = {.enabled = true},
                       .sheep_dog_pressure = {.enabled = true},
                       .sheep_dog_approach = {.enabled = true},
                       .sheep_dog_facing = {.enabled = true},
                       .sheep_temperament = {.enabled = true}},
    },
    {
        .name = "sheep-combined-influence-on",
        .definition = {.id = GameplayScenarioId::sheep_combined_influence_on,
                       .dog = {.initial_state = kCombinedInfluenceDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kCombinedInfluenceSheepStates,
                       .sheep_separation = {.enabled = true},
                       .sheep_dog_pressure = {.enabled = true},
                       .sheep_dog_approach = {.enabled = true},
                       .sheep_dog_facing = {.enabled = true},
                       .sheep_temperament = {.enabled = true},
                       .sheep_combined_influence = {.enabled = true}},
    },
    {
        .name = "sheep-motion-limit-off",
        .definition = {.id = GameplayScenarioId::sheep_motion_limit_off,
                       .dog = {.initial_state = kMotionLimitDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kMotionLimitSheepStates},
    },
    {
        .name = "sheep-motion-limit-on",
        .definition = {.id = GameplayScenarioId::sheep_motion_limit_on,
                       .dog = {.initial_state = kMotionLimitDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kMotionLimitSheepStates,
                       .sheep_motion_limit = {.enabled = true}},
    },
    {
        .name = "sheep-avoidance-off",
        .definition = {.id = GameplayScenarioId::sheep_avoidance_off,
                       .dog = {.initial_state = kAvoidanceDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kAvoidanceSheepStates},
    },
    {
        .name = "sheep-avoidance-on",
        .definition = {.id = GameplayScenarioId::sheep_avoidance_on,
                       .dog = {.initial_state = kAvoidanceDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kAvoidanceSheepStates,
                       .sheep_avoidance = {.enabled = true}},
    },
    {
        .name = "sheep-behavior-transitions-off",
        .definition = {.id = GameplayScenarioId::sheep_behavior_transitions_off,
                       .dog = {.initial_state = kBehaviorTransitionDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kBehaviorTransitionSheepStates,
                       .sheep_dog_pressure = {.radius = kBehaviorStimulusRadius},
                       .sheep_temperament = {.enabled = true}},
    },
    {
        .name = "sheep-behavior-transitions-on",
        .definition = {.id = GameplayScenarioId::sheep_behavior_transitions_on,
                       .dog = {.initial_state = kBehaviorTransitionDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kBehaviorTransitionSheepStates,
                       .sheep_dog_pressure = {.radius = kBehaviorStimulusRadius},
                       .sheep_temperament = {.enabled = true},
                       .sheep_behavior = {.enabled = true}},
    },
    {
        .name = "sheep-all-influences-diagnostic",
        .definition = {.id = GameplayScenarioId::sheep_all_influences_diagnostic,
                       .dog = {.initial_state = kAllInfluencesDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kAllInfluencesSheepStates,
                       .sheep_separation = {.enabled = true},
                       .sheep_attraction = {.enabled = true},
                       .sheep_alignment = {.enabled = true},
                       .sheep_dog_pressure = {.enabled = true},
                       .sheep_dog_approach = {.enabled = true},
                       .sheep_dog_facing = {.enabled = true},
                       .sheep_dog_line_of_sight = {.enabled = true},
                       .sheep_temperament = {.enabled = true},
                       .sheep_avoidance = {.enabled = true},
                       .sheep_combined_influence = {.enabled = true},
                       .sheep_motion_limit = {.enabled = true},
                       .sheep_behavior = {.enabled = true}},
    },
    {
        // The scale fixture. It exists to prove that an authoritative flock
        // larger than the accepted five runs the same rules over the same
        // contract, and every switch is on so that the grid rebuild, neighbour
        // selection, all seven steering terms, both motion limits, the combined
        // bound, the behaviour transition, and the paddock resolve are all
        // exercised at fifty members. Like `sheep-all-influences-diagnostic`,
        // and for the same reason, **it is a diagnostic rather than accepted
        // tuned gameplay**: no owner has reviewed the motion fifty sheep
        // produce, and no accepted design says the first playable has fifty.
        .name = "fifty-sheep-paddock",
        .definition = {.id = GameplayScenarioId::fifty_sheep_paddock,
                       .dog = {.initial_state = kFiftySheepDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .sheep_count = kFiftySheepCount,
                       .initial_sheep = kFiftySheepStates,
                       .sheep_separation = {.enabled = true},
                       .sheep_attraction = {.enabled = true},
                       .sheep_alignment = {.enabled = true},
                       .sheep_dog_pressure = {.enabled = true},
                       .sheep_dog_approach = {.enabled = true},
                       .sheep_dog_facing = {.enabled = true},
                       .sheep_dog_line_of_sight = {.enabled = true},
                       .sheep_temperament = {.enabled = true},
                       .sheep_avoidance = {.enabled = true},
                       .sheep_combined_influence = {.enabled = true},
                       .sheep_motion_limit = {.enabled = true},
                       .sheep_behavior = {.enabled = true}},
    },
}};

} // namespace

std::optional<GameplayScenarioDefinition> find_gameplay_scenario(std::string_view name) noexcept {
    for (const NamedGameplayScenario& scenario : kGameplayScenarios) {
        if (scenario.name == name) {
            return scenario.definition;
        }
    }
    return std::nullopt;
}

std::string_view gameplay_scenario_name(GameplayScenarioId scenario) noexcept {
    for (const NamedGameplayScenario& definition : kGameplayScenarios) {
        if (definition.definition.id == scenario) {
            return definition.name;
        }
    }
    return "unknown";
}

} // namespace wide_eye::game
