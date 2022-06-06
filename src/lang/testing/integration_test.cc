//
//  integration_tests.cc
//  Katara-tests
//
//  Created by Arne Philipeit on 6/25/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include <memory>

#include "gtest/gtest.h"
#include "src/common/filesystem/filesystem.h"
#include "src/common/filesystem/test_filesystem.h"
#include "src/ir/representation/program.h"
#include "src/lang/processors/docs/file_doc.h"
#include "src/lang/processors/docs/package_doc.h"
#include "src/lang/processors/ir/builder/ir_builder.h"
#include "src/lang/processors/packages/package.h"
#include "src/lang/processors/packages/package_manager.h"
#include "src/lang/representation/ast/ast.h"
#include "src/lang/representation/ast/ast_util.h"
#include "src/lang/representation/positions/positions.h"
#include "src/lang/representation/types/info_util.h"

namespace {

void LoadMainPackagesAndBuildProgram(lang::packages::PackageManager& pkg_manager) {
  // Load main package:
  lang::packages::Package* pkg = pkg_manager.LoadMainPackage("/");
  EXPECT_TRUE(pkg_manager.issue_tracker()->issues().empty());
  EXPECT_TRUE(pkg != nullptr);
  EXPECT_TRUE(pkg->issue_tracker().issues().empty());

  // Ensure lang debug and doc information can be generated:
  for (auto [name, ast_file] : pkg->ast_package()->files()) {
    lang::ast::NodeToTree(pkg_manager.file_set(), ast_file);
  }
  lang::types::InfoToText(pkg_manager.file_set(), pkg_manager.type_info());
  lang::docs::GenerateDocumentationForPackage(pkg, pkg_manager.file_set(), pkg_manager.type_info());

  // Generate IR:
  // TODO: uncomment when this no longer crashes
  // std::unique_ptr<ir::Program> program =
  //     lang::ir_builder::IRBuilder::TranslateProgram(pkg, pkg_manager.type_info());
  // EXPECT_TRUE(program != nullptr);

  // Ensure IR debug information can be generated:
  // program->ToString();
}

TEST(LangIntegrationTest, HandlesVectorDefinitionCorrectly) {
  std::string source = R"kat(
package vec

const Dim = 3
type Vector [Dim]int

func Add(a, b Vector) Vector {
   var res Vector
   for i := 0; i < Dim; i++ {
       res[i] = a[i] + b[i]
   }
   return res
}

func Negate(a Vector) (res Vector) {
   for i := 0; i < Dim; i++ {
       res[i] = -a[i]
   }
}

func Scale(a Vector, f int) Vector {
   res := a
   for i := 0; i < Dim; i++ {
       res[i] *= f
   }
   return res
}

func IsZero(a Vector) bool {
   for i := 0; i < Dim; i++ {
       if a[i] != 0 {
           return false
       }
   }
   return true
}

func Dot(a, b Vector) int {
   res := 0
   for i := 0; i < Dim; i++ {
       res += a[i] * b[i]
   }
   return res
}

func ForEachDim(a Vector, f func(i, x int)) {
   for i := 0; i < Dim; i++ {
       f(i, a[i])
   }
}

func ForEachDimRev(a Vector, f func(i, x int)) {
   for i := Dim - 1; i >= 0; i-- {
       f(i, a[i])
   }
}

type Func func(int) int

func Line(x, m, b int) int {
   return b + m * x
}

func LineFunc(m, b int) Func {
   return func(x int) int {
       return Line(x, m, b)
   }
}

func Parabola(x, a, b, c int) int {
   return x * x * a + x * b + c
}

func ParabolaFunc(a, b, c int) Func {
   return func(x int) int {
       return Parabola(x, a, b, c)
   }
}

func TransformedFunc(f Func, xOffset, yOffset, xScale, yScale int) Func {
   return func(x int) int {
       return (f((x - xOffset) * xScale) + yOffset) * yScale
   }
}
  )kat";
  common::TestFilesystem filesystem_;
  filesystem_.WriteContentsOfFile("vectors.kat", source);
  lang::packages::PackageManager pkg_manager(&filesystem_, /*stdlib_path=*/"", /*src_path=*/"");

  LoadMainPackagesAndBuildProgram(pkg_manager);
}

TEST(LangIntegrationTest, HandlesContainerDefinitionsCorrectly) {
  std::string source = R"kat(
package std

type List<T> interface {
    () Get(index int) T
    () Set(index int, value T)
    () Len() int
    
    () SubList(start, end int) List<T>
}

type ArrayList<T> struct{
    data []T
    start int
    length int
    capacity int
}

func NewArrayList<T>() *List<T> {

}

func (l *ArrayList<T>) Get(index int) T {
    return l.data[index]
}

func (l *ArrayList<T>) Set(index int, value T) {
    l.data[index] = value
}

type ComparisonResult int
type CompareFunc<T> func(a, b T) ComparisonResult

func SortInts(l List<int>) {
    Sort<int>(l, func(a, b int) ComparisonResult {
        return 0
    })
}

func Sort<T>(l List<T>, compareFunc CompareFunc<T>) {
    
}

type Map<K, V> interface {
    () Get(key K) V
    () Set(key K, value V)
    () Delete(key K) V
    
    () ForEach(f func(key K, value V))
}

type HashValue int64
type Hashable interface {
    () Hash() HashValue
}
type HashFunc<T> func(value T) HashValue

type HashMap<K, V> struct {
    data []struct{K; V;}
    hashFunc HashFunc<K>
}

func NewHashMap<K Hashable, V>() *HashMap<K, V> {
    return NewCustomHashMap<K, V>(func (value K) HashValue {
        return value.Hash()
    })
}

func NewCustomHashMap<K, V>(hashFunc HashFunc<K>) *HashMap<K, V> {
    
}

type Set<X> struct {
    values []X
}

func (s *Set<Y>) ForEach(f func(member Y)) {
    for i := 0; i < len(s.values); i++ {
        f(s.values[i])
    }
}

type Graph<T> struct {
    nodes Set<*Node<T>>
}

type Node<T> struct {
    value T
    neighbors Set<%Node<T>>
}

func (n *Node<T>) ForEachNeighbor(f func(neighbor %Node<T>)) {
    n.neighbors.ForEach(f)
}

type String = string

func (s String) Hash() HashValue {
    return HashValue(len(s))
}

func Test() {
    l := NewArrayList<int>()
    m := NewHashMap<String, List<int>>()
}
  )kat";
  common::TestFilesystem filesystem_;
  filesystem_.WriteContentsOfFile("containers.kat", source);
  lang::packages::PackageManager pkg_manager(&filesystem_, /*stdlib_path=*/"", /*src_path=*/"");

  LoadMainPackagesAndBuildProgram(pkg_manager);
}

TEST(LangIntegrationTest, HandlesInitOrderCorrectly) {
  std::string fmt_source = R"kat(
package fmt

func Println(text string) {
}

type Stringer interface {
    () String() string
}
  )kat";
  std::string main_source = R"kat(
package main

import (
    "fmt"
)

var (
  a = c + b
  b = f()
  c = f()
  d = 3
)

func f() int {
    d++
  return d
}

var x, y = g()

func g() (string, bool) {
    return "hello", false
}

func main() {
    fmt.Println("hello")
}
  )kat";
  common::TestFilesystem filesystem_;
  filesystem_.CreateDirectory("stdlib");
  filesystem_.CreateDirectory("stdlib/fmt");
  filesystem_.WriteContentsOfFile("stdlib/fmt/fmt.kat", fmt_source);
  filesystem_.WriteContentsOfFile("inits.kat", main_source);
  lang::packages::PackageManager pkg_manager(&filesystem_, /*stdlib_path=*/"stdlib",
                                             /*src_path=*/"");

  LoadMainPackagesAndBuildProgram(pkg_manager);
}

TEST(LangIntegrationTest, HandlesTypeEdgeCasesCorrectly) {
  std::string source = R"kat(
package main

type MyType int
const x MyType = 3
type MyVec [x]MyType
var test = MyVec{3,2,1}

type TypeA []TypeA
type TypeB<T> struct {
        x *TypeC<T>
}
type TypeC<T> struct {
        y *TypeB<T>
}
  )kat";
  common::TestFilesystem filesystem_;
  filesystem_.WriteContentsOfFile("types.kat", source);
  lang::packages::PackageManager pkg_manager(&filesystem_, /*stdlib_path=*/"", /*src_path=*/"");

  LoadMainPackagesAndBuildProgram(pkg_manager);
}

TEST(LangIntegrationTest, HandlesTypeMethodsCorrectly) {
  std::string fmt_source = R"kat(
package fmt

func Println(text string) {
}

type Stringer interface {
    () String() string
}
  )kat";
  std::string main_source = R"kat(
package main

import (
    "fmt"
)

type Int = int
type ComparisonResult int

const (
    Less ComparisonResult = iota
    Equal ComparisonResult
    Greater ComparisonResult
)

func <Int> Compare(a, b Int) ComparisonResult {
    if a < b {
        return Less
    } else if a == b {
        return Equal
    } else {
        return Greater
    }
}

func <Int> Min(nums []Int) Int {
    min := nums[0]
    for i := 1; i < len(nums); i++ {
        if nums[i] < min {
            min = nums[i]
        }
    }
    return min
}

func <Int> Max(nums []Int) Int {
    max := nums[0]
    for i := 1; i < len(nums); i++ {
        if nums[i] > max {
            max = nums[i]
        }
    }
    return max
}

type Comparable interface {
    <T> Compare(a, b T) ComparisonResult
    <T> Min(instances []T) T
    <T> Max(instances []T) T
}

type Range<T> struct {
    min, max T
}

func (r Range<T>) String() string {
    return "implement"
}

func RangeOf<T Comparable>(xs []T) Range<T> {
    min := T.Min(xs)
    max := T.Max(xs)
    return Range<T>{min, max}
}

func main() {
    fmt.Println(RangeOf<Int>([]Int{2, 1, 4, 3, 5, 4, 2}).String())
}
  )kat";
  common::TestFilesystem filesystem_;
  filesystem_.CreateDirectory("stdlib");
  filesystem_.CreateDirectory("stdlib/fmt");
  filesystem_.WriteContentsOfFile("stdlib/fmt/fmt.kat", fmt_source);
  filesystem_.WriteContentsOfFile("inits.kat", main_source);
  lang::packages::PackageManager pkg_manager(&filesystem_, /*stdlib_path=*/"stdlib",
                                             /*src_path=*/"");

  LoadMainPackagesAndBuildProgram(pkg_manager);
}

TEST(LangIntegrationTest, HandlesMultipleFilePackageCorrectly) {
  std::string fmt_source = R"kat(
package fmt

func Println(text string) {
}

type Stringer interface {
    () String() string
}
  )kat";
  std::string a_source = R"kat(
package xyz

import (
    "fmt"
)

type Bool = bool
type Int = int

func testNorm() {
    v := Vec3<Int>{1, 2, 3}
    fmt.Println(v.Norm().String())
}
  )kat";
  std::string b_source = R"kat(
package xyz

type Dim int

const (
    Three Dim = 3
    Four Dim = 4
)

type Vec3<T Number> [Three]T
type Vec4<T Number> [Four]T

func (v Vec3<T>) Norm() Vec3<T> {
    l := T.Sqrt(T.Add3(
        T.Mul(v[0], v[0]),
        T.Mul(v[1], v[1]),
        T.Mul(v[2], v[2])))
    return Vec3<T>{T.Div(v[0], l), T.Div(v[1], l), T.Div(v[2], l)}
}

func (v Vec3<T>) String() string {
    return "{" + v[0].String() + ", " + v[1].String() + ", " + v[2].String() + "}"
}
  )kat";
  std::string c_source = R"kat(
package main

import (
    "fmt"
)

type Number interface {
    <T> Add(a, b T) T
    <T> Add3(a, b, c T) T
    <T> Sub(a, b T) T
    <T> Mul(a, b T) T
    <T> Div(a, b T) T
    <T> Mod(a, b T) T
    <T> Sqrt(a T) T
    <T> String() string
}

func <Int> Add(a, b Int) Int {
    return a + b
}

func <Int> Add3(a, b, c Int) Int {
    return a + b + c
}

func <Int> Sub(a, b Int) Int {
    return a - b
}

func <Int> Mul(a, b Int) Int {
    return a * b
}

func <Int> Div(a, b Int) Int {
    return a / b
}

func <Int> Mod(a, b Int) Int {
    return a % b
}

func <Int> Sqrt(a Int) Int {
    return 0
}
  )kat";
  common::TestFilesystem filesystem_;
  filesystem_.CreateDirectory("stdlib");
  filesystem_.CreateDirectory("stdlib/fmt");
  filesystem_.WriteContentsOfFile("stdlib/fmt/fmt.kat", fmt_source);
  filesystem_.WriteContentsOfFile("a.kat", a_source);
  filesystem_.WriteContentsOfFile("b.kat", b_source);
  filesystem_.WriteContentsOfFile("c.kat", c_source);
  lang::packages::PackageManager pkg_manager(&filesystem_, /*stdlib_path=*/"stdlib",
                                             /*src_path=*/"");

  LoadMainPackagesAndBuildProgram(pkg_manager);
}

}  // namespace
