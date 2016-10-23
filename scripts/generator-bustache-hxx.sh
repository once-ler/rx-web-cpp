#!/bin/sh

out=include/bustache.hxx

cat include/bustache/detail/any_context.hpp | sed 's/^#include <bustache.*>//g' > $out
cat include/bustache/detail/variant.hpp | sed 's/^#include <bustache.*>//g' >> $out
cat include/bustache/ast.hpp | sed 's/^#include <bustache.*>//g' >> $out
cat include/bustache/format.hpp | sed 's/^#include <bustache.*>//g' >> $out
cat include/bustache/debug.hpp | sed 's/^#include <bustache.*>//g' >> $out
cat include/bustache/model.hpp | sed 's/^#include <bustache.*>//g' >> $out
cat include/bustache/generate.hpp | sed 's/^#include <bustache.*>//g' >> $out
cat include/bustache/generate/ostream.hpp | sed 's/^#include <bustache.*>//g' >> $out
cat include/bustache/generate/string.hpp | sed 's/^#include <bustache.*>//g' >> $out
cat src/format.cpp | sed 's/^#include <bustache.*>//g' >> $out
cat src/generate.cpp | sed 's/^#include <bustache.*>//g' >> $out
