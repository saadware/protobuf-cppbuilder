// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)

#include <google/protobuf/compiler/mock_code_generator.h>

#include <google/protobuf/testing/file.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/substitute.h>
#include <gtest/gtest.h>
#include <google/protobuf/stubs/stl_util-inl.h>

namespace google {
namespace protobuf {
namespace compiler {

// Returns the list of the names of files in all_files in the form of a
// comma-separated string.
std::string CommaSeparatedList(const std::vector<const FileDescriptor*> all_files) {
  std::vector<std::string> names;
  for (int i = 0; i < all_files.size(); i++) {
    names.push_back(all_files[i]->name());
  }
  return JoinStrings(names, ",");
}

static const char* kFirstInsertionPointName = "first_mock_insertion_point";
static const char* kSecondInsertionPointName = "second_mock_insertion_point";
static const char* kFirstInsertionPoint =
    "# @@protoc_insertion_point(first_mock_insertion_point) is here\n";
static const char* kSecondInsertionPoint =
    "  # @@protoc_insertion_point(second_mock_insertion_point) is here\n";

MockCodeGenerator::MockCodeGenerator(const std::string& name)
    : name_(name) {}

MockCodeGenerator::~MockCodeGenerator() {}

void MockCodeGenerator::ExpectGenerated(
    const std::string& name,
    const std::string& parameter,
    const std::string& insertions,
    const std::string& file,
    const std::string& first_message_name,
    const std::string& first_parsed_file_name,
    const std::string& output_directory) {
  std::string content;
  ASSERT_TRUE(File::ReadFileToString(
      output_directory + "/" + GetOutputFileName(name, file), &content));

  std::vector<std::string> lines;
  SplitStringUsing(content, "\n", &lines);

  while (!lines.empty() && lines.back().empty()) {
    lines.pop_back();
  }
  for (int i = 0; i < lines.size(); i++) {
    lines[i] += "\n";
  }

  std::vector<std::string> insertion_list;
  if (!insertions.empty()) {
    SplitStringUsing(insertions, ",", &insertion_list);
  }

  ASSERT_EQ(lines.size(), 3 + insertion_list.size() * 2);
  EXPECT_EQ(GetOutputFileContent(name, parameter, file,
                                 first_parsed_file_name, first_message_name),
            lines[0]);

  EXPECT_EQ(kFirstInsertionPoint, lines[1 + insertion_list.size()]);
  EXPECT_EQ(kSecondInsertionPoint, lines[2 + insertion_list.size() * 2]);

  for (int i = 0; i < insertion_list.size(); i++) {
    EXPECT_EQ(GetOutputFileContent(insertion_list[i], "first_insert",
                                   file, file, first_message_name),
              lines[1 + i]);
    // Second insertion point is indented, so the inserted text should
    // automatically be indented too.
    EXPECT_EQ("  " + GetOutputFileContent(insertion_list[i], "second_insert",
                                          file, file, first_message_name),
              lines[2 + insertion_list.size() + i]);
  }
}

bool MockCodeGenerator::Generate(
    const FileDescriptor* file,
    const std::string& parameter,
    GeneratorContext* context,
    std::string* error) const {
  for (int i = 0; i < file->message_type_count(); i++) {
    if (HasPrefixString(file->message_type(i)->name(), "MockCodeGenerator_")) {
      std::string command = StripPrefixString(file->message_type(i)->name(),
                                         "MockCodeGenerator_");
      if (command == "Error") {
        *error = "Saw message type MockCodeGenerator_Error.";
        return false;
      } else if (command == "Exit") {
        std::cerr << "Saw message type MockCodeGenerator_Exit." << std::endl;
        exit(123);
      } else if (command == "Abort") {
        std::cerr << "Saw message type MockCodeGenerator_Abort." << std::endl;
        abort();
      } else {
        GOOGLE_LOG(FATAL) << "Unknown MockCodeGenerator command: " << command;
      }
    }
  }

  if (HasPrefixString(parameter, "insert=")) {
    std::vector<std::string> insert_into;
    SplitStringUsing(StripPrefixString(parameter, "insert="),
                     ",", &insert_into);

    for (int i = 0; i < insert_into.size(); i++) {
      {
        scoped_ptr<io::ZeroCopyOutputStream> output(
            context->OpenForInsert(
              GetOutputFileName(insert_into[i], file),
              kFirstInsertionPointName));
        io::Printer printer(output.get(), '$');
        printer.PrintRaw(GetOutputFileContent(name_, "first_insert",
                                              file, context));
        if (printer.failed()) {
          *error = "MockCodeGenerator detected write error.";
          return false;
        }
      }

      {
        scoped_ptr<io::ZeroCopyOutputStream> output(
            context->OpenForInsert(
              GetOutputFileName(insert_into[i], file),
              kSecondInsertionPointName));
        io::Printer printer(output.get(), '$');
        printer.PrintRaw(GetOutputFileContent(name_, "second_insert",
                                              file, context));
        if (printer.failed()) {
          *error = "MockCodeGenerator detected write error.";
          return false;
        }
      }
    }
  } else {
    scoped_ptr<io::ZeroCopyOutputStream> output(
        context->Open(GetOutputFileName(name_, file)));

    io::Printer printer(output.get(), '$');
    printer.PrintRaw(GetOutputFileContent(name_, parameter,
                                          file, context));
    printer.PrintRaw(kFirstInsertionPoint);
    printer.PrintRaw(kSecondInsertionPoint);

    if (printer.failed()) {
      *error = "MockCodeGenerator detected write error.";
      return false;
    }
  }

  return true;
}

std::string MockCodeGenerator::GetOutputFileName(const std::string& generator_name,
                                            const FileDescriptor* file) {
  return GetOutputFileName(generator_name, file->name());
}

std::string MockCodeGenerator::GetOutputFileName(const std::string& generator_name,
                                            const std::string& file) {
  return file + ".MockCodeGenerator." + generator_name;
}

std::string MockCodeGenerator::GetOutputFileContent(
    const std::string& generator_name,
    const std::string& parameter,
    const FileDescriptor* file,
    GeneratorContext *context) {
  std::vector<const FileDescriptor*> all_files;
  context->ListParsedFiles(&all_files);
  return GetOutputFileContent(
      generator_name, parameter, file->name(),
      CommaSeparatedList(all_files),
      file->message_type_count() > 0 ?
          file->message_type(0)->name() : "(none)");
}

std::string MockCodeGenerator::GetOutputFileContent(
    const std::string& generator_name,
    const std::string& parameter,
    const std::string& file,
    const std::string& parsed_file_list,
    const std::string& first_message_name) {
  return strings::Substitute("$0: $1, $2, $3, $4\n",
      generator_name, parameter, file,
      first_message_name, parsed_file_list);
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
