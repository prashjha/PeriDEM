/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 *
 * inp::Field generates the reader, the writer, the printed form and the schema
 * of a deck from one declaration of its fields. These checks cover that
 * generation on a deck defined here, so that a failure names the table
 * machinery rather than one of the decks that use it.
 */

#include "inp/deckField.h"

#include <iostream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool ok, const std::string &what) {
  ++g_checks;
  if (ok)
    return;
  ++g_failures;
  std::cerr << "  FAIL: " << what << "\n";
}

/*! @brief Fails unless the call throws with a message containing @p contains */
template <typename Call>
void checkThrows(Call call, const std::string &contains,
                 const std::string &what) {
  ++g_checks;
  try {
    call();
  } catch (const std::exception &e) {
    if (std::string(e.what()).find(contains) != std::string::npos)
      return;
    ++g_failures;
    std::cerr << "  FAIL: " << what << ": message was " << e.what();
    return;
  }
  ++g_failures;
  std::cerr << "  FAIL: " << what << ": nothing was thrown\n";
}

/*! @brief A deck with one field of each kind the tables have to handle. */
struct SampleDeck {
  std::size_t d_count = 2;
  double d_length = 1.5;
  std::string d_method = "central_difference";
  bool d_write = true;
  double d_optional = -1.;

  static const std::vector<inp::Field<SampleDeck>> &fields() {
    static const std::vector<inp::Field<SampleDeck>> f = {
        inp::field(&SampleDeck::d_count, "Count", std::size_t(2),
                   "How many of them", {{}, std::size_t(1), std::size_t(3)}),
        inp::field(&SampleDeck::d_length, "Length", 1.5, "Length of the side"),
        inp::field(&SampleDeck::d_method, "Method",
                   std::string("central_difference"), "Time discretization",
                   {{"central_difference", "velocity_verlet"}, {}, {}}),
        inp::field(&SampleDeck::d_write, "Write", true, "Write the output"),
        // Absent from the block when it is not positive, which is how the
        // decks treat a value that another field supplies instead.
        inp::field<SampleDeck, double>(
            &SampleDeck::d_optional, "Optional", -1., "Set only when positive",
            {}, [](const double &v) { return v > 0.; }),
    };
    return f;
  }
};

void testDefaultsAndSchema() {
  std::cout << "defaults and schema\n";
  const json d = inp::defaultsJson(SampleDeck::fields());
  check(d.at("Count") == 2, "Count default is written");
  check(d.at("Method") == "central_difference", "Method default is written");
  check(d.find("Optional") == d.end(),
        "a field whose emit condition is false is absent");

  const json s = inp::schemaJson(SampleDeck::fields());
  check(s.size() == 5, "schema reports one entry per field");
  check(s.at(0).at("key") == "Count", "schema reports the key");
  check(s.at(0).at("type") == "size_t", "schema reports the type");
  check(s.at(0).at("default") == 2, "schema reports the default");
  check(s.at(0).at("doc") == "How many of them", "schema reports the doc");
  check(s.at(4).find("default") == s.at(4).end(),
        "schema omits the default of a field that is not written");
}

void testReadAndWrite() {
  std::cout << "read and write\n";
  json j = inp::applyGiven(json{{"Count", 3}, {"Length", 2.25}},
                           SampleDeck::fields());
  SampleDeck d;
  inp::readFields(d, j, SampleDeck::fields());
  check(d.d_count == 3, "Count is read");
  check(d.d_length == 2.25, "Length is read");
  check(d.d_method == "central_difference", "Method keeps its default");
  check(d.d_write, "Write keeps its default");

  json out = json::object();
  inp::writeFields(d, out, SampleDeck::fields());
  check(out == j, "writing what was read gives the block back");

  // A block with no keys leaves every member at its default.
  SampleDeck empty;
  inp::readFields(empty, json::object(), SampleDeck::fields());
  check(empty.d_count == 2, "an absent key leaves the default");
}

void testAccepted() {
  std::cout << "accepted values and bounds\n";
  SampleDeck d;
  checkThrows(
      [&] {
        inp::readFields(d, json{{"Method", "backward_euler"}},
                        SampleDeck::fields());
      },
      "must be one of", "a value outside the accepted set is rejected");
  checkThrows(
      [&] {
        inp::readFields(d, json{{"Count", 7}}, SampleDeck::fields());
      },
      "must be at most", "a value above the upper bound is rejected");
  checkThrows(
      [&] {
        inp::readFields(d, json{{"Count", 0}}, SampleDeck::fields());
      },
      "must be at least", "a value below the lower bound is rejected");
}

void testKeysAreNamed() {
  std::cout << "keys are named and checked\n";
  checkThrows([] { inp::applyGiven(json{{"Coun", 3}}, SampleDeck::fields()); },
              "Closest declared field is Count",
              "a misspelled key names the closest field");
  checkThrows([] { inp::applyGiven(json{{"Horizon", 1.}}, SampleDeck::fields()); },
              "no field named Horizon", "an unrelated key is rejected");
  checkThrows([] { inp::applyGiven(json{1, 2, 3}, SampleDeck::fields()); },
              "name and value pairs",
              "a list of values without names is rejected");

  // A key the deck handles outside the table is accepted when declared.
  json j = inp::applyGiven(json{{"Nested", json::object()}},
                           SampleDeck::fields(), {"Nested"});
  check(j.find("Nested") != j.end(), "a key named in extra is kept");
}

void testPrint() {
  std::cout << "printed form\n";
  SampleDeck d;
  std::ostringstream oss;
  inp::printFields(d, oss, SampleDeck::fields(), "  ");
  const std::string s = oss.str();
  check(s.find("  Count = 2") != std::string::npos,
        "the printed form has Count after the tab prefix");
  check(s.find("  Method = central_difference") != std::string::npos,
        "the printed form has Method after the tab prefix");
}

void testEditDistance() {
  std::cout << "edit distance\n";
  check(inp::editDistance("Count", "Count") == 0, "equal strings");
  check(inp::editDistance("Coun", "Count") == 1, "one deletion");
  check(inp::editDistance("", "abc") == 3, "an empty string");
}

void run(void (*fn)(), const std::string &name) {
  try {
    fn();
  } catch (const std::exception &e) {
    ++g_checks;
    ++g_failures;
    std::cerr << "  FAIL: " << name << " threw: " << e.what() << "\n";
  }
}

} // namespace

int main() {
  std::cout << "Deck field table\n"
            << "----------------\n";
  run(testDefaultsAndSchema, "defaults and schema");
  run(testReadAndWrite, "read and write");
  run(testAccepted, "accepted values");
  run(testKeysAreNamed, "keys are named");
  run(testPrint, "printed form");
  run(testEditDistance, "edit distance");

  std::cout << "----------------\n"
            << g_checks - g_failures << " / " << g_checks << " checks passed\n";
  return g_failures == 0 ? 0 : 1;
}
