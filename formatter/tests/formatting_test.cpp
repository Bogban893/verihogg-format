#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "data/format_style.h"
#include "data/lex_context.h"
#include "formatter.h"

namespace {

class FormattingTest : public ::testing::Test {
 protected:
  auto formatText(std::string_view source,
                  format::FormatStyle style = format::FormatStyle::defaults())
      -> std::string {
    tokens_ = ctx_.lex_string(source);
    return format::format(tokens_, style, ctx_.source_text()).formatted_text;
  }

 private:
  LexContext ctx_;
  std::vector<slang::parsing::Token> tokens_;
};

}  // namespace

// =============================================================================
// LowRISC Coding Style Guide - spacing compliance.
//
// Every spacing rule is tested twice:
//   - <Rule>Preserve  : the input is already correct; the formatter must not
//                       perturb it.
//   - <Rule>Normalize : the input has the wrong spacing; the formatter must
//                       rewrite it into the canonical form.
//
// The Normalize tests are the ones that actually exercise the rules; the
// Preserve tests guard against regressions (e.g. kCaseLabelColon leaking
// into non-case colon contexts).
// =============================================================================

// -----------------------------------------------------------------------------
// Binary operators: space on both sides.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, BinaryOperatorsPreserve) {
  const auto out =
      formatText("module m; initial a = b + c * d - e / f; endmodule");
  EXPECT_NE(out.find("b + c"), std::string::npos);
  EXPECT_NE(out.find("c * d"), std::string::npos);
  EXPECT_NE(out.find("d - e"), std::string::npos);
  EXPECT_NE(out.find("e / f"), std::string::npos);
}

TEST_F(FormattingTest, BinaryOperatorsNormalize) {
  const auto out = formatText(
      "module m; initial begin a=b+c; d=e-f; g=h*i; j=k/l; end endmodule");
  EXPECT_NE(out.find("b + c"), std::string::npos);
  EXPECT_NE(out.find("e - f"), std::string::npos);
  EXPECT_NE(out.find("h * i"), std::string::npos);
  EXPECT_NE(out.find("k / l"), std::string::npos);
  EXPECT_EQ(out.find("b+c"), std::string::npos);
  EXPECT_EQ(out.find("e-f"), std::string::npos);
  EXPECT_EQ(out.find("h*i"), std::string::npos);
  EXPECT_EQ(out.find("k/l"), std::string::npos);
}

// -----------------------------------------------------------------------------
// Unary operators: no space between operator and operand.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, UnaryOperatorsPreserve) {
  const auto out = formatText("module m; initial a = ~b & !c | -d; endmodule");
  EXPECT_NE(out.find("~b"), std::string::npos);
  EXPECT_NE(out.find("!c"), std::string::npos);
  EXPECT_NE(out.find("-d"), std::string::npos);
  EXPECT_EQ(out.find("~ b"), std::string::npos);
  EXPECT_EQ(out.find("! c"), std::string::npos);
}

TEST_F(FormattingTest, UnaryOperatorsNormalize) {
  const auto out = formatText("module m; initial a = ~ b & ! c | - d; endmodule");
  EXPECT_NE(out.find("~b"), std::string::npos);
  EXPECT_NE(out.find("!c"), std::string::npos);
  EXPECT_NE(out.find("-d"), std::string::npos);
  EXPECT_EQ(out.find("~ b"), std::string::npos);
  EXPECT_EQ(out.find("! c"), std::string::npos);
}

// -----------------------------------------------------------------------------
// Ternary: space around '?' and ':'.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, TernaryPreserve) {
  const auto out = formatText("module m; assign x = a ? b : c; endmodule");
  EXPECT_NE(out.find("a ? b : c"), std::string::npos);
}

TEST_F(FormattingTest, TernaryNormalize) {
  const auto out = formatText("module m; assign x = a?b:c; endmodule");
  EXPECT_NE(out.find("a ? b : c"), std::string::npos);
  EXPECT_EQ(out.find("a?b"), std::string::npos);
  EXPECT_EQ(out.find("b:c"), std::string::npos);
}

// -----------------------------------------------------------------------------
// Packed dimensions: space before '[', compact range [7:0].
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, PackedDimensionPreserve) {
  const auto out = formatText("module m; logic [7:0] a; endmodule");
  EXPECT_NE(out.find("logic [7:0]"), std::string::npos);
  EXPECT_EQ(out.find("[7 : 0]"), std::string::npos);
}

TEST_F(FormattingTest, PackedDimensionNormalize) {
  const auto out = formatText("module m; logic [7 : 0]a; endmodule");
  EXPECT_NE(out.find("logic [7:0]"), std::string::npos);
  EXPECT_NE(out.find("logic [7:0] a"), std::string::npos);
  EXPECT_EQ(out.find("[7 : 0]"), std::string::npos);
}

// -----------------------------------------------------------------------------
// Multiple packed dimensions: no space between.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, MultiplePackedDimensionsPreserve) {
  const auto out = formatText("module m; logic [7:0][3:0] a; endmodule");
  EXPECT_NE(out.find("[7:0][3:0]"), std::string::npos);
}

TEST_F(FormattingTest, MultiplePackedDimensionsNormalize) {
  const auto out = formatText("module m; logic [7:0] [3:0] a; endmodule");
  EXPECT_NE(out.find("[7:0][3:0]"), std::string::npos);
  EXPECT_EQ(out.find("[7:0] [3:0]"), std::string::npos);
}

// -----------------------------------------------------------------------------
// Unpacked dimensions: no space between identifier and '['.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, UnpackedDimensionsPreserve) {
  const auto out = formatText("module m; bit a[512]; endmodule");
  EXPECT_NE(out.find("a[512]"), std::string::npos);
  EXPECT_EQ(out.find("a [512]"), std::string::npos);
}

TEST_F(FormattingTest, UnpackedDimensionsNormalize) {
  const auto out = formatText("module m; bit a [512]; endmodule");
  EXPECT_NE(out.find("a[512]"), std::string::npos);
  EXPECT_EQ(out.find("a [512]"), std::string::npos);
}

TEST_F(FormattingTest, UnpackedDynamicArrayNormalize) {
  const auto out = formatText("module m; logic a []; endmodule");
  EXPECT_NE(out.find("a[]"), std::string::npos);
  EXPECT_EQ(out.find("a []"), std::string::npos);
}

// TODO: formatter keeps a space between the identifier and the queue
// dimension ("b [$]" instead of "b[$]"). Enable once queue dimensions
// are recognized as part of the unpacked dimension list.
// TEST_F(FormattingTest, UnpackedQueueNormalize) {
//   const auto out = formatText("module m; int b [$]; endmodule");
//   EXPECT_NE(out.find("b[$]"), std::string::npos);
//   EXPECT_EQ(out.find("b [$]"), std::string::npos);
// }

// -----------------------------------------------------------------------------
// Packed and unpacked together.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, PackedAndUnpackedPreserve) {
  const auto out = formatText("module m; logic [7:0] data[128][2]; endmodule");
  EXPECT_NE(out.find("logic [7:0] data[128][2]"), std::string::npos);
}

// -----------------------------------------------------------------------------
// Indexed part-select: compact.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, IndexedPartSelectPreserve) {
  const auto out = formatText(
      "module m; assign y = x[3+:2]; assign z = x[7-:4]; endmodule");
  EXPECT_NE(out.find("x[3+:2]"), std::string::npos);
  EXPECT_NE(out.find("x[7-:4]"), std::string::npos);
}

// TODO: formatter keeps spaces around indexed part-select operators
// ("x[3 +: 2]" instead of "x[3+:2]"). Enable once the indexed operators
// are recognized and compacted inside '[...]'.
// TEST_F(FormattingTest, IndexedPartSelectNormalize) {
//   const auto out = formatText(
//       "module m; assign y = x[3 +: 2]; assign z = x[7 -: 4]; endmodule");
//   EXPECT_NE(out.find("x[3+:2]"), std::string::npos);
//   EXPECT_NE(out.find("x[7-:4]"), std::string::npos);
//   EXPECT_EQ(out.find("[3 +: 2]"), std::string::npos);
//   EXPECT_EQ(out.find("[7 -: 4]"), std::string::npos);
// }

// -----------------------------------------------------------------------------
// Function call: no space before '('.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, FunctionCallPreserve) {
  const auto out = formatText("module m; initial f(x); endmodule");
  EXPECT_NE(out.find("f(x)"), std::string::npos);
  EXPECT_EQ(out.find("f (x)"), std::string::npos);
}

TEST_F(FormattingTest, FunctionCallNormalize) {
  const auto out = formatText("module m; initial f (x); endmodule");
  EXPECT_NE(out.find("f(x)"), std::string::npos);
  EXPECT_EQ(out.find("f (x)"), std::string::npos);
}

// -----------------------------------------------------------------------------
// System function call: no space before '('.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, SystemCallPreserve) {
  const auto out = formatText("module m; initial $display(x); endmodule");
  EXPECT_NE(out.find("$display(x)"), std::string::npos);
  EXPECT_EQ(out.find("$display (x)"), std::string::npos);
}

TEST_F(FormattingTest, SystemCallNormalize) {
  const auto out = formatText("module m; initial $display (x); endmodule");
  EXPECT_NE(out.find("$display(x)"), std::string::npos);
  EXPECT_EQ(out.find("$display (x)"), std::string::npos);
}

// -----------------------------------------------------------------------------
// Macro call: no space before '('.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, MacroCallPreserve) {
  const auto out =
      formatText("module m; initial `uvm_error(ID, \"x\"); endmodule");
  EXPECT_NE(out.find("`uvm_error(ID"), std::string::npos);
  EXPECT_EQ(out.find("`uvm_error (ID"), std::string::npos);
}

TEST_F(FormattingTest, MacroCallNormalize) {
  const auto out =
      formatText("module m; initial `uvm_error (ID, \"x\"); endmodule");
  EXPECT_NE(out.find("`uvm_error(ID"), std::string::npos);
  EXPECT_EQ(out.find("`uvm_error (ID"), std::string::npos);
}

// -----------------------------------------------------------------------------
// Comma-delimited lists: exactly one space after ','.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, CommaPreserve) {
  const auto out = formatText("module m; initial {a, b, c} = x; endmodule");
  EXPECT_NE(out.find("{a, b, c}"), std::string::npos);
  EXPECT_EQ(out.find("{a,b,c}"), std::string::npos);
}

TEST_F(FormattingTest, CommaNormalize) {
  const auto out = formatText("module m; initial {a,b,c} = x; endmodule");
  EXPECT_NE(out.find("{a, b, c}"), std::string::npos);
  EXPECT_EQ(out.find("{a,b,c}"), std::string::npos);
}

// -----------------------------------------------------------------------------
// Case item colon: no space before, at least one space after (lowRISC style).
// Primary coverage for the kCaseLabelColon change.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, CaseItemColonPreserve) {
  const auto out = formatText(
      "module m; always_comb case(a) 0: b = 1; default: b = 0; endcase "
      "endmodule");
  EXPECT_NE(out.find("0: b"), std::string::npos);
  EXPECT_NE(out.find("default: b"), std::string::npos);
  EXPECT_EQ(out.find("0 : b"), std::string::npos);
  EXPECT_EQ(out.find("default : b"), std::string::npos);
}

TEST_F(FormattingTest, CaseItemColonNormalizeSpaceBefore) {
  const auto out = formatText(
      "module m; always_comb case(a) 0 : b = 1; default : b = 0; endcase "
      "endmodule");
  EXPECT_NE(out.find("0: b"), std::string::npos);
  EXPECT_NE(out.find("default: b"), std::string::npos);
  EXPECT_EQ(out.find("0 : b"), std::string::npos);
  EXPECT_EQ(out.find("default : b"), std::string::npos);
}

TEST_F(FormattingTest, CaseItemColonWithBeginPreserve) {
  const auto out = formatText(
      "module m; always_comb case(a) 0: begin b = 1; end default: begin b = 0; "
      "end endcase endmodule");
  EXPECT_NE(out.find("0: begin"), std::string::npos);
  EXPECT_NE(out.find("default: begin"), std::string::npos);
  EXPECT_EQ(out.find("0 : begin"), std::string::npos);
  EXPECT_EQ(out.find("default : begin"), std::string::npos);
}

TEST_F(FormattingTest, CaseItemColonWithBeginNormalize) {
  const auto out = formatText(
      "module m; always_comb case(a) 0 : begin b = 1; end default : begin b = 0; "
      "end endcase endmodule");
  EXPECT_NE(out.find("0: begin"), std::string::npos);
  EXPECT_NE(out.find("default: begin"), std::string::npos);
  EXPECT_EQ(out.find("0 : begin"), std::string::npos);
  EXPECT_EQ(out.find("default : begin"), std::string::npos);
}

TEST_F(FormattingTest, CaseItemColonMultipleLabelsNormalize) {
  const auto out = formatText(
      "module m; always_comb case(a) 1, 2 : b = 1; endcase endmodule");
  EXPECT_NE(out.find("2: b"), std::string::npos);
  EXPECT_EQ(out.find("2 : b"), std::string::npos);
}

// -----------------------------------------------------------------------------
// Non-case colons must not be affected by kCaseLabelColon.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, NonCaseColonTernaryPreserve) {
  const auto out = formatText("module m; assign x = a ? b : c; endmodule");
  EXPECT_NE(out.find("a ? b : c"), std::string::npos);
  EXPECT_EQ(out.find("a ? b: c"), std::string::npos);
}

TEST_F(FormattingTest, NonCaseColonRangePreserve) {
  const auto out = formatText("module m; logic [7:0] a; assign b = a[3:0]; endmodule");
  EXPECT_NE(out.find("[7:0]"), std::string::npos);
  EXPECT_NE(out.find("a[3:0]"), std::string::npos);
  EXPECT_EQ(out.find("[7 : 0]"), std::string::npos);
}

// -----------------------------------------------------------------------------
// Integer base literals: compact.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, IntegerBasePreserve) {
  const auto out = formatText("module m; logic [7:0] a = 8'hA0; endmodule");
  EXPECT_NE(out.find("8'"), std::string::npos);
  EXPECT_EQ(out.find("8 '"), std::string::npos);
  EXPECT_EQ(out.find("8' h"), std::string::npos);
}

// -----------------------------------------------------------------------------
// Type cast: compact.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, TypeCastPreserve) {
  const auto out = formatText("module m; initial a = int'(b); endmodule");
  EXPECT_NE(out.find("int'(b)"), std::string::npos);
  EXPECT_EQ(out.find("int' (b)"), std::string::npos);
}

// -----------------------------------------------------------------------------
// Unbased unsized literals: compact.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, UnbasedUnsizedPreserve) {
  const auto out =
      formatText("module m; initial a = '0; initial b = '1; endmodule");
  EXPECT_NE(out.find("'0"), std::string::npos);
  EXPECT_NE(out.find("'1"), std::string::npos);
}

// -----------------------------------------------------------------------------
// Replication: compact.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, ReplicationPreserve) {
  const auto out =
      formatText("module m; initial a = {8{1'b0}}; endmodule");
  EXPECT_NE(out.find("{8{1'b0}}"), std::string::npos);
}

TEST_F(FormattingTest, ReplicationNormalize) {
  const auto out =
      formatText("module m; initial a = { 8 { 1'b0 } }; endmodule");
  EXPECT_NE(out.find("{8{1'b0}}"), std::string::npos);
  EXPECT_EQ(out.find("{ 8"), std::string::npos);
}

// -----------------------------------------------------------------------------
// Qualified name with '::': no spaces.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, DoubleColonPreserve) {
  const auto out = formatText("module m; initial a = pkg::x; endmodule");
  EXPECT_NE(out.find("pkg::x"), std::string::npos);
  EXPECT_EQ(out.find("pkg :: x"), std::string::npos);
}

// TODO: formatter keeps spaces around the scope-resolution operator
// ("pkg :: x" instead of "pkg::x"). Enable once '::' is compacted.
// TEST_F(FormattingTest, DoubleColonNormalize) {
//   const auto out = formatText("module m; initial a = pkg :: x; endmodule");
//   EXPECT_NE(out.find("pkg::x"), std::string::npos);
//   EXPECT_EQ(out.find("pkg :: x"), std::string::npos);
// }

// -----------------------------------------------------------------------------
// Field access '.': no spaces.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, DotPreserve) {
  const auto out = formatText("module m; initial a = b.c; endmodule");
  EXPECT_NE(out.find("b.c"), std::string::npos);
  EXPECT_EQ(out.find("b . c"), std::string::npos);
}

TEST_F(FormattingTest, DotNormalize) {
  const auto out = formatText("module m; initial a = b . c; endmodule");
  EXPECT_NE(out.find("b.c"), std::string::npos);
  EXPECT_EQ(out.find("b . c"), std::string::npos);
}

// -----------------------------------------------------------------------------
// Event control '@': no spaces.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, AtSignPreserve) {
  const auto out =
      formatText("module m; always @(posedge clk) begin end endmodule");
  EXPECT_NE(out.find("@(posedge clk)"), std::string::npos);
  EXPECT_EQ(out.find("@ (posedge clk)"), std::string::npos);
}

TEST_F(FormattingTest, AtSignNormalize) {
  const auto out =
      formatText("module m; always @ (posedge clk) begin end endmodule");
  EXPECT_NE(out.find("@(posedge clk)"), std::string::npos);
  EXPECT_EQ(out.find("@ (posedge clk)"), std::string::npos);
}

// -----------------------------------------------------------------------------
// Assignment pattern colon: no space before, space after.
// -----------------------------------------------------------------------------
// TODO: formatter inserts a space before ':' in assignment patterns
// ("'{src : src}" instead of "'{src: src}"). This is a pre-existing gap
// in kColon handling; fix should mirror the kCaseLabelColon treatment
// (no whitespace before, one space after).
// TEST_F(FormattingTest, AssignmentPatternColonPreserve) {
//   const auto out =
//       formatText("module m; initial a = '{src: src, dst: dst}; endmodule");
//   EXPECT_NE(out.find("src: src"), std::string::npos);
//   EXPECT_NE(out.find("dst: dst"), std::string::npos);
//   EXPECT_EQ(out.find("src : src"), std::string::npos);
//   EXPECT_EQ(out.find("dst : dst"), std::string::npos);
// }
//
// TEST_F(FormattingTest, AssignmentPatternColonNormalize) {
//   const auto out =
//       formatText("module m; initial a = '{src : src, dst : dst}; endmodule");
//   EXPECT_NE(out.find("src: src"), std::string::npos);
//   EXPECT_NE(out.find("dst: dst"), std::string::npos);
//   EXPECT_EQ(out.find("src : src"), std::string::npos);
//   EXPECT_EQ(out.find("dst : dst"), std::string::npos);
// }

// -----------------------------------------------------------------------------
// Named block labels: one space before AND after ':'.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, LabelColonPreserve) {
  const auto out =
      formatText("module m; initial begin : foo a = 1; end endmodule");
  EXPECT_NE(out.find("begin : foo"), std::string::npos);
  EXPECT_EQ(out.find("begin: foo"), std::string::npos);
  EXPECT_EQ(out.find("begin :foo"), std::string::npos);
}

TEST_F(FormattingTest, LabelColonNormalize) {
  const auto out =
      formatText("module m; initial begin: foo a = 1; end endmodule");
  EXPECT_NE(out.find("begin : foo"), std::string::npos);
  EXPECT_EQ(out.find("begin: foo"), std::string::npos);
}

// -----------------------------------------------------------------------------
// Complex combinations - all rules at once.
// -----------------------------------------------------------------------------
TEST_F(FormattingTest, ComplexTernaryPreserve) {
  const auto out =
      formatText("module m; assign x = (a & b) == c ? d + e : ~f; endmodule");
  EXPECT_NE(out.find("(a & b)"), std::string::npos);
  EXPECT_NE(out.find("d + e"), std::string::npos);
  EXPECT_NE(out.find("? d + e :"), std::string::npos);
  EXPECT_EQ(out.find("?d"), std::string::npos);
  EXPECT_EQ(out.find(":~f"), std::string::npos);
}

TEST_F(FormattingTest, ComplexPackedDimensions) {
  const auto out =
      formatText("module m; logic [WIDTH-1:0] data; endmodule");
  EXPECT_NE(out.find("[WIDTH-1:0]"), std::string::npos);
  EXPECT_EQ(out.find("[WIDTH - 1 : 0]"), std::string::npos);
}

TEST_F(FormattingTest, ComplexConcatReplicationPreserve) {
  const auto out =
      formatText("module m; initial a = {8{1'b0}, {2{b}}, c}; endmodule");
  EXPECT_NE(out.find("{8{"), std::string::npos);
  EXPECT_NE(out.find("{2{"), std::string::npos);
  EXPECT_EQ(out.find("{ 8{"), std::string::npos);
  EXPECT_EQ(out.find("{ 2{"), std::string::npos);
}

TEST_F(FormattingTest, ComplexCaseItemsPreserve) {
  const auto out = formatText(
      "module m; always_comb begin case(op) 2'b00: res = a + b; 2'b01: res = a "
      "& b; default: res = '0; endcase end endmodule");
  EXPECT_NE(out.find("2'b00: res"), std::string::npos);
  EXPECT_NE(out.find("default: res"), std::string::npos);
  EXPECT_NE(out.find("a + b"), std::string::npos);
  EXPECT_EQ(out.find("2'b00 : res"), std::string::npos);
  EXPECT_EQ(out.find("default : res"), std::string::npos);
}

TEST_F(FormattingTest, ComplexCaseItemsNormalize) {
  const auto out = formatText(
      "module m; always_comb begin case(op) 2'b00 : res = a + b; 2'b01 : res = "
      "a & b; default : res = '0; endcase end endmodule");
  EXPECT_NE(out.find("2'b00: res"), std::string::npos);
  EXPECT_NE(out.find("2'b01: res"), std::string::npos);
  EXPECT_NE(out.find("default: res"), std::string::npos);
  EXPECT_EQ(out.find("2'b00 : res"), std::string::npos);
  EXPECT_EQ(out.find("2'b01 : res"), std::string::npos);
  EXPECT_EQ(out.find("default : res"), std::string::npos);
}

// TODO: formatter removes spaces around operators inside index expressions
// ("mem[addr+1]" instead of "mem[addr + 1]"). Index expressions are not
// declarations, so the "whitespace around all binary operators" rule applies.
// Enable once index expressions are formatted with spaced operators.
// TEST_F(FormattingTest, ComplexArrayIndexing) {
//   const auto out = formatText(
//      "module m; logic [7:0] mem[255:0]; assign y = mem[addr + 1]; endmodule");
//  EXPECT_NE(out.find("addr + 1"), std::string::npos);
//  EXPECT_EQ(out.find("addr+1"), std::string::npos);
// }

TEST_F(FormattingTest, ComplexBitwiseOperators) {
  const auto out =
      formatText("module m; assign x = (a & b) | (c ^ d); endmodule");
  EXPECT_NE(out.find("a & b"), std::string::npos);
  EXPECT_NE(out.find("c ^ d"), std::string::npos);
}

TEST_F(FormattingTest, ComplexShiftOperators) {
  const auto out = formatText(
      "module m; assign x = a << 2; assign y = b >> 1; endmodule");
  EXPECT_NE(out.find("a << 2"), std::string::npos);
  EXPECT_NE(out.find("b >> 1"), std::string::npos);
}

TEST_F(FormattingTest, ComplexComparisonOperators) {
  const auto out = formatText(
      "module m; always_comb if (a == b && c != d) begin end endmodule");
  EXPECT_NE(out.find("a == b"), std::string::npos);
  EXPECT_NE(out.find("c != d"), std::string::npos);
}

TEST_F(FormattingTest, ComplexParameterInstantiation) {
  const auto out = formatText(
      "module m; foo #(.WIDTH(8), .DEPTH(16)) u_foo (.clk(clk), .rst(rst)); "
      "endmodule");
  EXPECT_NE(out.find("foo #("), std::string::npos);
  EXPECT_EQ(out.find("foo#("), std::string::npos);
  EXPECT_NE(out.find(".WIDTH(8)"), std::string::npos);
}

TEST_F(FormattingTest, ComplexArrayDeclarations) {
  const auto out = formatText(
      "module m; logic [31:0] data[0:255]; logic [7:0][3:0] matrix[16]; "
      "endmodule");
  EXPECT_NE(out.find("data[0:255]"), std::string::npos);
  EXPECT_NE(out.find("[7:0][3:0]"), std::string::npos);
}

TEST_F(FormattingTest, ComplexNestedCallsAndIndexing) {
  const auto out =
      formatText("module m; assign x = $clog2(WIDTH) + mem[idx]; endmodule");
  EXPECT_NE(out.find("$clog2(WIDTH)"), std::string::npos);
  EXPECT_NE(out.find("mem[idx]"), std::string::npos);
}

TEST_F(FormattingTest, ComplexConcatenationWithOperators) {
  const auto out = formatText("module m; assign x = {a + b, c & d}; endmodule");
  EXPECT_NE(out.find("a + b"), std::string::npos);
  EXPECT_NE(out.find("c & d"), std::string::npos);
}

TEST_F(FormattingTest, ComplexModuleInstantiation) {
  const auto out = formatText(
      "module m; scr1_memory_tb_ahb #(.ADDR_WIDTH(32), .DATA_WIDTH(32)) "
      "i_mem(.clk(clk), .rst_n(rst_n), .addr(addr)); endmodule");
  EXPECT_NE(out.find(".ADDR_WIDTH(32)"), std::string::npos);
  EXPECT_NE(out.find(") i_mem"), std::string::npos);
}

TEST_F(FormattingTest, ComplexPackedArraysWithOperatorsInRanges) {
  const auto out = formatText(
      "module m; logic [(DATA_WIDTH * 2)-1:0] wide_bus; endmodule");
  EXPECT_NE(out.find("* 2"), std::string::npos);
  EXPECT_NE(out.find("[("), std::string::npos);
}

TEST_F(FormattingTest, ComplexUnaryAndBinary) {
  const auto out =
      formatText("module m; assign out = ~(a & ~b) | (!c); endmodule");
  EXPECT_NE(out.find("~(a & ~b)"), std::string::npos);
  EXPECT_NE(out.find("a & ~b"), std::string::npos);
}

TEST_F(FormattingTest, ComplexExpressionPrecedence) {
  const auto out =
      formatText("module m; assign result = (a + b) * (c - d) / e; endmodule");
  EXPECT_NE(out.find("(a + b)"), std::string::npos);
  EXPECT_NE(out.find("(c - d)"), std::string::npos);
}

TEST_F(FormattingTest, ComplexDimensionsAndCalls) {
  const auto out = formatText(
      "module m; logic [7:0] ram[1023:0]; assign data = "
      "ram[$unsigned(addr[9:0])]; endmodule");
  EXPECT_NE(out.find("ram[1023:0]"), std::string::npos);
  EXPECT_NE(out.find("$unsigned(addr[9:0])"), std::string::npos);
}

TEST_F(FormattingTest, ComplexMacroWithArgs) {
  const auto out = formatText(
      "module m; initial `ASSERT((a == b), \"error\"); endmodule");
  EXPECT_NE(out.find("`ASSERT(("), std::string::npos);
  EXPECT_NE(out.find("(a == b), \"error\""), std::string::npos);
}

// -----------------------------------------------------------------------------
  // begin / end placement (lowRISC style).
  //
  // Style guide rules covered here:
  //   - "Use begin and end unless the whole statement fits on a single line."
  //   - "begin must be on the same line as the preceding keyword, and ends
  //      the line. end must start a new line. end else begin must be together
  //      on one line."
  //   - "The only exception is if end has a label, a following else should be
  //      on a new line."
  //   - "Case items: begin and end may be omitted if the entire case item fits
  //      on a single line."
  //   - "Generated blocks must be named; a space is placed between begin and
  //      the code block name."
  //
  // The formatter joins/splits end/else correctly; inserting begin/end around
  // a wrapped statement that can fit on a single line is not required (the
  // formatter may collapse such statements to one line).
  // -----------------------------------------------------------------------------

TEST_F(FormattingTest, BeginEndSingleLineIfPreserve) {
  // A single-line if/else may omit begin/end.
  const auto out = formatText(
      "module m; initial if (a) b = 1;\n    else b = 0; endmodule");
  EXPECT_NE(out.find("if (a) b = 1;"), std::string::npos);
  EXPECT_NE(out.find("else b = 0;"), std::string::npos);
}

TEST_F(FormattingTest, BeginEndSingleLineAlwaysPreserve) {
  // A single-line always_ff may omit begin/end.
  const auto out = formatText(
      "module m; logic q, d, clk; always_ff @(posedge clk) q <= d; endmodule");
  EXPECT_NE(out.find("always_ff @(posedge clk) q <= d;"), std::string::npos);
}

TEST_F(FormattingTest, BeginEndWrappedBlockPreserve) {
  // A wrapped procedural block must use begin/end.
  const auto out = formatText(
      "module m; logic q, d, clk; always_ff @(posedge clk) begin\n    q <= d;\n"
      "  end\nendmodule");
  EXPECT_NE(out.find("begin"), std::string::npos);
  EXPECT_NE(out.find("end"), std::string::npos);
}

TEST_F(FormattingTest, BeginOnKeywordLinePreserve) {
  // begin is on the same line as the preceding keyword.
  const auto out = formatText(
      "module m; initial if (a) begin\n    b = 1;\n  end\nendmodule");
  EXPECT_NE(out.find("if (a) begin"), std::string::npos);
  EXPECT_EQ(out.find("if (a)\n"), std::string::npos);
}

TEST_F(FormattingTest, EndElseBeginSameLinePreserve) {
  // end else begin are on one line.
  const auto out = formatText(
      "module m; initial if (a) begin\n    b = 1;\n  end else begin\n"
      "    b = 0;\n  end\nendmodule");
  EXPECT_NE(out.find("end else begin"), std::string::npos);
  EXPECT_EQ(out.find("end\n  else begin"), std::string::npos);
}

TEST_F(FormattingTest, EndLabeledFollowedByElsePreserve) {
  // Exception: when end has a label, else goes on a new line.
  const auto out = formatText(
      "module m; initial if (a) begin : blk_a\n    b = 1;\n  end : blk_a\n"
      "  else begin : blk_b\n    b = 0;\n  end : blk_b\nendmodule");
  EXPECT_NE(out.find("end : blk_a"), std::string::npos);
  EXPECT_NE(out.find("else begin : blk_b"), std::string::npos);
  EXPECT_NE(out.find("end : blk_b"), std::string::npos);
  EXPECT_EQ(out.find("end : blk_a else"), std::string::npos);
}

TEST_F(FormattingTest, BeginLabeledSpacingPreserve) {
  // Labeled begin/end: one space before AND after the colon.
  const auto out =
      formatText("module m; initial begin : foo\n    a = 1;\n  end : foo\n"
                 "endmodule");
  EXPECT_NE(out.find("begin : foo"), std::string::npos);
  EXPECT_NE(out.find("end : foo"), std::string::npos);
  EXPECT_EQ(out.find("begin: foo"), std::string::npos);
  EXPECT_EQ(out.find("begin :foo"), std::string::npos);
}

TEST_F(FormattingTest, BeginLabeledSpacingNormalize) {
  // Missing whitespace around the label colon is inserted.
  const auto out =
      formatText("module m; initial begin: foo\n    a = 1;\n  end: foo\n"
                 "endmodule");
  EXPECT_NE(out.find("begin : foo"), std::string::npos);
  EXPECT_NE(out.find("end : foo"), std::string::npos);
  EXPECT_EQ(out.find("begin: foo"), std::string::npos);
  EXPECT_EQ(out.find("end: foo"), std::string::npos);
}

TEST_F(FormattingTest, CaseItemSingleLineNoBeginPreserve) {
  // Case items that fit on one line may omit begin/end.
  const auto out = formatText(
      "module m; always_comb case (s) 2'b00: y = 8'h00; default: y = 8'hff; "
      "endcase endmodule");
  EXPECT_NE(out.find("2'b00: y = 8'h00;"), std::string::npos);
  EXPECT_NE(out.find("default: y = 8'hff;"), std::string::npos);
}

TEST_F(FormattingTest, CaseItemWrappedRequiresBeginPreserve) {
  // Wrapped case items use begin on the same line as the case expression.
  const auto out = formatText(
      "module m; always_comb case (s) 2'b00: begin\n    y = 8'h00;\n"
      "    z = 8'h01;\n  end\n  default: begin\n    y = 8'hff;\n  end\n"
      "endcase endmodule");
  EXPECT_NE(out.find("2'b00: begin"), std::string::npos);
  EXPECT_NE(out.find("default: begin"), std::string::npos);
}

TEST_F(FormattingTest, GenerateNamedBlockPreserve) {
  // Generated blocks must be named; space between begin and the name.
  const auto out = formatText(
      "module m; generate if (N > 0) begin : gen_pos\n    assign x = 1;\n"
      "  end\nendgenerate endmodule");
  EXPECT_NE(out.find("begin : gen_pos"), std::string::npos);
  EXPECT_EQ(out.find("begin: gen_pos"), std::string::npos);
}

TEST_F(FormattingTest, GenerateNamedBlockSpacingNormalize) {
  // Missing whitespace before the generate block name is inserted.
  const auto out = formatText(
      "module m; generate if (N > 0) begin: gen_pos\n    assign x = 1;\n"
      "  end\nendgenerate endmodule");
  EXPECT_NE(out.find("begin : gen_pos"), std::string::npos);
  EXPECT_EQ(out.find("begin: gen_pos"), std::string::npos);
}

// -----------------------------------------------------------------------------
  // begin/end Normalize tests.
  // -----------------------------------------------------------------------------

  // end and else must be on the same line (unlabeled end).
  TEST_F(FormattingTest, EndElseSeparateLinesNormalize) {
    const auto out = formatText(
        "module m; initial if (a) begin\n    b = 1;\n  end\n"
        "  else begin\n    b = 0;\n  end\nendmodule");
    EXPECT_NE(out.find("end else begin"), std::string::npos);
    EXPECT_EQ(out.find("end\n  else"), std::string::npos);
  }

  // Labeled end forces else to a new line.
  TEST_F(FormattingTest, EndLabeledElseSameLineNormalize) {
    const auto out = formatText(
        "module m; initial if (a) begin : blk_a\n    b = 1;\n"
        "  end : blk_a else begin : blk_b\n    b = 0;\n"
        "  end : blk_b\nendmodule");
    EXPECT_NE(out.find("end : blk_a\n"), std::string::npos);
    EXPECT_NE(out.find("else begin : blk_b"), std::string::npos);
    EXPECT_EQ(out.find("end : blk_a else"), std::string::npos);
  }
