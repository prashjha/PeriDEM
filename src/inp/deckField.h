/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_DECKFIELD_H
#define INP_DECKFIELD_H

#include "util/io.h"
#include "util/json.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace inp {

/*!
 * @brief One field of a deck, declared once
 *
 * A deck class states its fields in a table of these. Reading a block, writing
 * one, printing the deck and reporting its schema are generated from the
 * table, so the reader and the writer cannot name different keys or carry
 * different defaults. Three defects of that kind were found in this code
 * before the tables existed.
 *
 * A field that does not map to one key stays in the deck's own readDerived and
 * writeDerived. Nested blocks such as Discretization_Type, values computed
 * from other fields such as the time step, and keys that exclude one another
 * such as Kn and V_Max are of that kind.
 */
template <class Deck> struct Field {

  /*! @brief Key in the JSON block */
  std::string key;

  /*! @brief One line describing the field, reported by the schema */
  std::string doc;

  /*! @brief Name of the type, reported by the schema */
  std::string type;

  /*! @brief Reads the key into the member, or leaves the default in place */
  std::function<void(Deck &, const json &)> read;

  /*! @brief Writes the member under the key, if it is to be written */
  std::function<void(const Deck &, json &)> write;

  /*! @brief Writes the default under the key, if it is to be written */
  std::function<void(json &)> writeDefault;

  /*! @brief Appends the key and its value to a stream, after a tab prefix */
  std::function<void(const Deck &, std::ostringstream &, const std::string &)>
      print;

  /*! @brief Throws if the value is one the field does not accept */
  std::function<void(const json &)> check;

  /*! @brief Whether a value is written rather than left out of the block */
  std::function<bool(const json &)> emit;
};

/*! @brief Name of the type of a field, reported by the schema */
template <class T> inline std::string typeName() { return "value"; }
/*! @copydoc typeName() */
template <> inline std::string typeName<bool>() { return "bool"; }
/*! @copydoc typeName() */
template <> inline std::string typeName<int>() { return "int"; }
/*! @copydoc typeName() */
template <> inline std::string typeName<std::size_t>() { return "size_t"; }
/*! @copydoc typeName() */
template <> inline std::string typeName<double>() { return "double"; }
/*! @copydoc typeName() */
template <> inline std::string typeName<std::string>() { return "string"; }
/*! @copydoc typeName() */
template <> inline std::string typeName<std::vector<double>>() {
  return "double[]";
}
/*! @copydoc typeName() */
template <> inline std::string typeName<std::vector<std::string>>() {
  return "string[]";
}
/*! @copydoc typeName() */
template <> inline std::string typeName<std::vector<std::size_t>>() {
  return "size_t[]";
}

/*! @brief Appends a value to a stream */
template <class T>
void printValue(std::ostringstream &oss, const T &v) {
  oss << v;
}

/*! @brief Appends the entries of a vector, separated by commas */
template <class T>
void printValue(std::ostringstream &oss, const std::vector<T> &v) {
  oss << util::io::printStr<T>(v, 0);
}

/*!
 * @brief What a field accepts
 *
 * An empty list and unset bounds accept any value. The checks are applied when
 * the field is read, so an invalid deck fails at the point it is read rather
 * than at the point the value is first used.
 */
template <class T> struct Accepts {

  /*! @brief Accepted values, or empty when any value is accepted */
  std::vector<T> values = {};

  /*! @brief Lower bound, applied when it is set */
  std::optional<T> min = std::nullopt;

  /*! @brief Upper bound, applied when it is set */
  std::optional<T> max = std::nullopt;
};

/*!
 * @brief Declares a field that maps one key to one member
 *
 * @param m Pointer to the member
 * @param key Key in the JSON block
 * @param def Value used when the key is absent
 * @param doc One line describing the field
 * @param accepts What the field accepts
 * @param emitIf Written only when this is true of the value. Use it for a key
 * the deck omits rather than writes at its default, such as Horizon, which is
 * absent when the horizon is set by Horizon_Mesh_Ratio instead
 * @return field The field
 */
template <class Deck, class T>
Field<Deck> field(
    T Deck::*m, std::string key, T def, std::string doc,
    Accepts<T> accepts = {},
    std::function<bool(const T &)> emitIf = [](const T &) { return true; }) {

  // Accepted values and bounds apply to the scalar types. A field holding a
  // list is checked by the deck if it needs to be.
  constexpr bool checkable =
      std::is_arithmetic_v<T> || std::is_same_v<T, std::string>;

  auto check = [key, accepts](const T &v) {
    if constexpr (checkable) {
      if (!accepts.values.empty() &&
          std::find(accepts.values.begin(), accepts.values.end(), v) ==
              accepts.values.end()) {
        std::ostringstream oss;
        oss << "Error: " << key << " must be one of:";
        for (const auto &a : accepts.values)
          oss << " " << a;
        oss << ". Given value is " << v << ".\n";
        throw std::runtime_error(oss.str());
      }
      if (accepts.min.has_value() && v < accepts.min.value()) {
        std::ostringstream oss;
        oss << "Error: " << key << " must be at least " << accepts.min.value()
            << ". Given value is " << v << ".\n";
        throw std::runtime_error(oss.str());
      }
      if (accepts.max.has_value() && accepts.max.value() < v) {
        std::ostringstream oss;
        oss << "Error: " << key << " must be at most " << accepts.max.value()
            << ". Given value is " << v << ".\n";
        throw std::runtime_error(oss.str());
      }
    }
  };

  return Field<Deck>{
      key, std::move(doc), typeName<T>(),
      [m, key, def, check](Deck &d, const json &j) {
        d.*m = j.value(key, def);
        check(d.*m);
      },
      [m, key, emitIf](const Deck &d, json &j) {
        if (emitIf(d.*m))
          j[key] = d.*m;
      },
      [key, def, emitIf](json &j) {
        if (emitIf(def))
          j[key] = def;
      },
      [m, key](const Deck &d, std::ostringstream &oss,
               const std::string &tab) {
        oss << tab << key << " = ";
        printValue(oss, d.*m);
        oss << std::endl;
      },
      [check](const json &v) { check(v.get<T>()); },
      [emitIf](const json &v) { return emitIf(v.get<T>()); }};
}

/*!
 * @brief Declares a field that has no default and must be present
 *
 * @param m Pointer to the member
 * @param key Key in the JSON block
 * @param doc One line describing the field
 * @param accepts What the field accepts
 * @return field The field
 */
template <class Deck, class T>
Field<Deck> requiredField(T Deck::*m, std::string key, std::string doc,
                          Accepts<T> accepts = {}) {
  auto f = field(m, key, T{}, std::move(doc), accepts);
  f.read = [m, key, accepts, f](Deck &d, const json &j) {
    if (j.find(key) == j.end())
      throw std::runtime_error("Error: " + key + " is required.\n");
    json one = json::object();
    one[key] = j.at(key);
    f.read(d, one);
  };
  return f;
}

/*! @brief Reads every field of the table from the block */
template <class Deck>
void readFields(Deck &d, const json &j, const std::vector<Field<Deck>> &fs) {
  for (const auto &f : fs)
    f.read(d, j);
}

/*! @brief Writes every field of the table into the block */
template <class Deck>
void writeFields(const Deck &d, json &j, const std::vector<Field<Deck>> &fs) {
  for (const auto &f : fs)
    f.write(d, j);
}

/*!
 * @brief Appends every field of the table to a stream
 *
 * @param d The deck
 * @param oss Stream to append to
 * @param fs Field table of the deck
 * @param tab Prefix written before each field
 */
template <class Deck>
void printFields(const Deck &d, std::ostringstream &oss,
                 const std::vector<Field<Deck>> &fs,
                 const std::string &tab = "") {
  for (const auto &f : fs)
    f.print(d, oss, tab);
}

/*! @brief The block with every field at its default */
template <class Deck>
json defaultsJson(const std::vector<Field<Deck>> &fs) {
  json j = json::object();
  for (const auto &f : fs)
    f.writeDefault(j);
  return j;
}

/*!
 * @brief Key, type, default and description of every field
 *
 * The Python interface builds its arguments from this, so that a field added
 * to the table needs no change on the Python side.
 *
 * @param fs Field table of the deck
 * @return schema One object per field
 */
template <class Deck> json schemaJson(const std::vector<Field<Deck>> &fs) {
  json out = json::array();
  const json defaults = defaultsJson(fs);
  for (const auto &f : fs) {
    json entry = {{"key", f.key}, {"type", f.type}, {"doc", f.doc}};
    if (defaults.find(f.key) != defaults.end())
      entry["default"] = defaults.at(f.key);
    out.push_back(entry);
  }
  return out;
}

/*!
 * @brief A quantity that can be given in more than one way
 *
 * The horizon is given either as Horizon or as Horizon_Mesh_Ratio. The contact
 * stiffness is given either as Kn or as V_Max with Delta_Max. Before these
 * groups were declared, the choice was recorded by giving the unused key a
 * value outside its range, -1 or a number below 1e-10, and the reader decided
 * which had been meant by comparing against that. A value outside the range is
 * then not available as a value, and a deck that sets neither key or both is
 * accepted.
 */
struct OneOf {

  /*! @brief What the group gives, named in the error message */
  std::string what;

  /*! @brief Keys of the group */
  std::vector<std::string> keys;

  /*! @brief Whether one of the keys has to be given */
  bool required = true;
};

/*!
 * @brief Checks that each group has exactly one of its keys
 *
 * @param block The block to check
 * @param groups Groups of the deck
 */
inline void checkGroups(const json &block, const std::vector<OneOf> &groups) {
  for (const auto &g : groups) {
    std::vector<std::string> present;
    for (const auto &k : g.keys)
      if (block.find(k) != block.end())
        present.push_back(k);

    if (present.size() == 1 || (present.empty() && !g.required))
      continue;

    std::ostringstream oss;
    oss << "Error: give exactly one of";
    for (const auto &k : g.keys)
      oss << " " << k;
    oss << " for " << g.what << ". ";
    if (present.empty())
      oss << "None was given.\n";
    else {
      oss << "These were given:";
      for (const auto &k : present)
        oss << " " << k;
      oss << ".\n";
    }
    throw std::runtime_error(oss.str());
  }
}

/*!
 * @brief Number of single character edits between two strings
 *
 * Used to name the closest key when a caller supplies one that is not
 * declared.
 *
 * @param a First string
 * @param b Second string
 * @return n Edit distance
 */
inline std::size_t editDistance(const std::string &a, const std::string &b) {
  std::vector<std::size_t> prev(b.size() + 1), cur(b.size() + 1);
  for (std::size_t j = 0; j <= b.size(); ++j)
    prev[j] = j;
  for (std::size_t i = 1; i <= a.size(); ++i) {
    cur[0] = i;
    for (std::size_t j = 1; j <= b.size(); ++j)
      cur[j] = std::min({prev[j] + 1, cur[j - 1] + 1,
                         prev[j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1)});
    prev = cur;
  }
  return prev[b.size()];
}

/*!
 * @brief Rejects a key that is not among those named
 *
 * A deck is built by naming keys. A key that is not declared would otherwise
 * be dropped without a message, and the run would proceed with the default in
 * place of the value the caller asked for. A key close to a declared one is
 * reported with the name it was probably meant to be.
 *
 * @param given Keys the caller supplied
 * @param names Every key the deck accepts
 */
inline void checkNames(const json &given,
                       const std::vector<std::string> &names) {
  if (given.is_null() || given.empty())
    return;
  if (!given.is_object())
    throw std::runtime_error(
        "Error: deck values must be given as name and value pairs.\n");

  for (const auto &item : given.items()) {
    const std::string key = item.key();
    if (std::find(names.begin(), names.end(), key) != names.end())
      continue;

    std::string closest;
    std::size_t best = std::string::npos;
    for (const auto &n : names) {
      const std::size_t d = editDistance(key, n);
      if (d < best) {
        best = d;
        closest = n;
      }
    }
    std::ostringstream oss;
    oss << "Error: no field named " << key << ".";
    if (best <= 3)
      oss << " Closest declared field is " << closest << ".";
    oss << "\nDeclared fields are:";
    for (const auto &n : names)
      oss << " " << n;
    oss << "\n";
    throw std::runtime_error(oss.str());
  }
}

/*!
 * @brief Rejects a key that no field declares
 *
 * @param given Keys the caller supplied
 * @param fs Field table of the deck
 * @param extra Keys the deck handles outside the table
 */
template <class Deck>
void checkKeys(const json &given, const std::vector<Field<Deck>> &fs,
               const std::vector<std::string> &extra = {}) {
  std::vector<std::string> names;
  names.reserve(fs.size() + extra.size());
  for (const auto &f : fs)
    names.push_back(f.key);
  names.insert(names.end(), extra.begin(), extra.end());
  checkNames(given, names);
}

/*!
 * @brief The default block with the given keys replaced
 *
 * The name of every key and the value of every field are checked here, so
 * that a deck is rejected where it is built rather than where it is read.
 *
 * @param given Keys and values to replace, checked against the table
 * @param fs Field table of the deck
 * @param extra Keys the deck handles outside the table
 * @return j The block
 */
template <class Deck>
json applyGiven(const json &given, const std::vector<Field<Deck>> &fs,
                const std::vector<std::string> &extra = {}) {
  checkKeys(given, fs, extra);
  json j = defaultsJson(fs);
  if (!given.is_object())
    return j;

  for (const auto &item : given.items()) {
    bool emit = true;
    for (const auto &f : fs)
      if (f.key == item.key()) {
        f.check(item.value());
        emit = f.emit(item.value());
      }
    // A field that is not written at its default is not written when the
    // same value is given, so that the block does not depend on whether the
    // caller named the field.
    if (emit)
      j[item.key()] = item.value();
    else
      j.erase(item.key());
  }
  return j;
}

} // namespace inp

#endif // INP_DECKFIELD_H
