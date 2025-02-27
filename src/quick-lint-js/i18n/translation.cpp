// Copyright (C) 2020  Matthew "strager" Glazar
// See end of file for extended copyright information.

#include <cerrno>
#include <clocale>
#include <cstring>
#include <optional>
#include <quick-lint-js/assert.h>
#include <quick-lint-js/i18n/translation.h>
#include <quick-lint-js/port/have.h>
#include <quick-lint-js/port/warning.h>
#include <string>
#include <string_view>
#include <vector>

using namespace std::literals::string_view_literals;

QLJS_WARNING_IGNORE_GCC("-Wuseless-cast")

namespace quick_lint_js {
Translator qljs_messages;

namespace {
std::vector<std::string> split_on(const char* s, char separator) {
  std::vector<std::string> locales;
  for (;;) {
    const char* sep = std::strchr(s, separator);
    if (sep) {
      locales.emplace_back(s, sep);
      s = sep + 1;
    } else {
      locales.emplace_back(s);
      break;
    }
  }
  return locales;
}

std::vector<std::string> get_user_locale_preferences() {
  // This lookup order roughly mimics GNU gettext.

  int category =
#if QLJS_HAVE_LC_MESSAGES
      LC_MESSAGES
#else
      LC_ALL
#endif
      ;
  const char* locale = std::setlocale(category, nullptr);
  if (locale && locale == "C"sv) {
    return {};
  }

  const char* language_env = ::getenv("LANGUAGE");
  if (language_env && language_env[0] != '\0') {
    return split_on(language_env, ':');
  }

  // TODO(strager): Determine the language using macOS' and Windows' native
  // APIs. See GNU gettext's _nl_language_preferences_default.

  return {locale};
}

void initialize_locale() {
  if (!std::setlocale(LC_ALL, "")) {
    std::fprintf(stderr, "warning: failed to set locale: %s\n",
                 std::strerror(errno));
  }
}
}

void initialize_translations_from_environment() {
  initialize_locale();
  if (!qljs_messages.use_messages_from_locales(get_user_locale_preferences())) {
    qljs_messages.use_messages_from_source_code();
  }
}

void initialize_translations_from_locale(const char* locale_name) {
  initialize_locale();
  if (!qljs_messages.use_messages_from_locale(locale_name)) {
    qljs_messages.use_messages_from_source_code();
  }
}

void Translator::use_messages_from_source_code() {
  // See NOTE[untranslated-locale-slot].
  this->locale_index_ = translation_table_locale_count;
}

bool Translator::use_messages_from_locale(const char* locale_name) {
  std::optional<int> locale_index =
      find_locale(translation_data.locale_table, locale_name);
  if (locale_index.has_value()) {
    this->locale_index_ = *locale_index;
    return true;
  }
  return false;
}

bool Translator::use_messages_from_locales(
    const std::vector<std::string>& locale_names) {
  for (const std::string& locale : locale_names) {
    if (locale == "C" || locale == "POSIX") {
      // Stop seaching. C/POSIX locale takes priority. See GNU gettext.
      break;
    }
    bool found_messages = this->use_messages_from_locale(locale.c_str());
    if (found_messages) {
      return true;
    }
  }
  return false;
}

const Char8* Translator::translate(const Translatable_Message& message) {
  // If the following assertion fails, it's likely that
  // translation-table-generated.h is out of date.
  QLJS_ASSERT(message.valid());

  std::uint16_t mapping_index = message.translation_table_mapping_index();
  const Translation_Table::Mapping_Entry& mapping =
      translation_data.mapping_table[mapping_index];
  std::uint32_t string_offset = mapping.string_offsets[this->locale_index_];
  if (string_offset == 0) {
    // The string has no translation.
    string_offset = mapping.string_offsets[translation_table_locale_count];
    QLJS_ASSERT(string_offset != 0);
  }
  return translation_data.string_table + string_offset;
}
}

// quick-lint-js finds bugs in JavaScript programs.
// Copyright (C) 2020  Matthew "strager" Glazar
//
// This file is part of quick-lint-js.
//
// quick-lint-js is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// quick-lint-js is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with quick-lint-js.  If not, see <https://www.gnu.org/licenses/>.
