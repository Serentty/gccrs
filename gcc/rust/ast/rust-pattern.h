// Copyright (C) 2020 Free Software Foundation, Inc.

// This file is part of GCC.

// GCC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3, or (at your option) any later
// version.

// GCC is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.

// You should have received a copy of the GNU General Public License
// along with GCC; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

#ifndef RUST_AST_PATTERN_H
#define RUST_AST_PATTERN_H

#include "rust-ast.h"

namespace Rust {
namespace AST {
// Literal pattern AST node (comparing to a literal)
class LiteralPattern : public Pattern
{
  Literal lit;
  /* make literal have a type given by enum, etc. rustc uses an extended form of
   * its literal token implementation */
  // TODO: literal representation - use LiteralExpr? or another thing?

  // Minus prefixed to literal (if integer or floating-point)
  bool has_minus;
  // Actually, this might be a good place to use a template.

  Location locus;

public:
  std::string as_string () const override;

  // Constructor for a literal pattern
  LiteralPattern (Literal lit, Location locus, bool has_minus = false)
    : lit (std::move (lit)), has_minus (has_minus), locus (locus)
  {}

  LiteralPattern (std::string val, Literal::LitType type, Location locus,
		  bool has_minus = false)
    : lit (Literal (std::move (val), type, PrimitiveCoreType::CORETYPE_STR)),
      has_minus (has_minus), locus (locus)
  {}

  Location get_locus () const { return locus; }
  Location get_locus_slow () const final override { return get_locus (); }

  void accept_vis (ASTVisitor &vis) override;

protected:
  /* Use covariance to implement clone function as returning this object rather
   * than base */
  virtual LiteralPattern *clone_pattern_impl () const override
  {
    return new LiteralPattern (*this);
  }
};

// Identifier pattern AST node (bind value matched to a variable)
class IdentifierPattern : public Pattern
{
  Identifier variable_ident;
  bool is_ref;
  bool is_mut;

  // bool has_pattern;
  std::unique_ptr<Pattern> to_bind;

  Location locus;

public:
  std::string as_string () const override;

  // Returns whether the IdentifierPattern has a pattern to bind.
  bool has_pattern_to_bind () const { return to_bind != nullptr; }

  // Constructor
  IdentifierPattern (Identifier ident, Location locus, bool is_ref = false,
		     bool is_mut = false,
		     std::unique_ptr<Pattern> to_bind = nullptr)
    : Pattern (), variable_ident (std::move (ident)), is_ref (is_ref),
      is_mut (is_mut), to_bind (std::move (to_bind)), locus (locus)
  {}

  IdentifierPattern (NodeId node_id, Identifier ident, Location locus,
		     bool is_ref = false, bool is_mut = false,
		     std::unique_ptr<Pattern> to_bind = nullptr)
    : Pattern (), variable_ident (std::move (ident)), is_ref (is_ref),
      is_mut (is_mut), to_bind (std::move (to_bind)), locus (locus)
  {
    this->node_id = node_id;
  }

  // Copy constructor with clone
  IdentifierPattern (IdentifierPattern const &other)
    : variable_ident (other.variable_ident), is_ref (other.is_ref),
      is_mut (other.is_mut), locus (other.locus)
  {
    node_id = other.node_id;
    // fix to get prevent null pointer dereference

    if (other.to_bind != nullptr)
      to_bind = other.to_bind->clone_pattern ();
  }

  // Overload assignment operator to use clone
  IdentifierPattern &operator= (IdentifierPattern const &other)
  {
    variable_ident = other.variable_ident;
    is_ref = other.is_ref;
    is_mut = other.is_mut;
    locus = other.locus;
    node_id = other.node_id;

    // fix to prevent null pointer dereference
    if (other.to_bind != nullptr)
      to_bind = other.to_bind->clone_pattern ();
    else
      to_bind = nullptr;

    return *this;
  }

  // default move semantics
  IdentifierPattern (IdentifierPattern &&other) = default;
  IdentifierPattern &operator= (IdentifierPattern &&other) = default;

  Location get_locus () const { return locus; }
  Location get_locus_slow () const final override { return get_locus (); }

  void accept_vis (ASTVisitor &vis) override;

  // TODO: is this better? Or is a "vis_pattern" better?
  std::unique_ptr<Pattern> &get_pattern_to_bind ()
  {
    rust_assert (has_pattern_to_bind ());
    return to_bind;
  }

  Identifier get_ident () const { return variable_ident; }

  bool get_is_mut () const { return is_mut; }
  bool get_is_ref () const { return is_ref; }

protected:
  /* Use covariance to implement clone function as returning this object rather
   * than base */
  IdentifierPattern *clone_pattern_impl () const override
  {
    return new IdentifierPattern (*this);
  }
};

// AST node for using the '_' wildcard "match any value" pattern
class WildcardPattern : public Pattern
{
  Location locus;

public:
  std::string as_string () const override { return std::string (1, '_'); }

  WildcardPattern (Location locus) : locus (locus) {}

  Location get_locus () const { return locus; }
  Location get_locus_slow () const final override { return get_locus (); }

  void accept_vis (ASTVisitor &vis) override;

protected:
  /* Use covariance to implement clone function as returning this object rather
   * than base */
  WildcardPattern *clone_pattern_impl () const override
  {
    return new WildcardPattern (*this);
  }
};

// Base range pattern bound (lower or upper limit) - abstract
class RangePatternBound
{
public:
  virtual ~RangePatternBound () {}

  // Unique pointer custom clone function
  std::unique_ptr<RangePatternBound> clone_range_pattern_bound () const
  {
    return std::unique_ptr<RangePatternBound> (
      clone_range_pattern_bound_impl ());
  }

  virtual std::string as_string () const = 0;

  virtual void accept_vis (ASTVisitor &vis) = 0;

protected:
  // pure virtual as RangePatternBound is abstract
  virtual RangePatternBound *clone_range_pattern_bound_impl () const = 0;
};

// Literal-based pattern bound
class RangePatternBoundLiteral : public RangePatternBound
{
  Literal literal;
  /* Can only be a char, byte, int, or float literal - same impl here as
   * previously */

  // Minus prefixed to literal (if integer or floating-point)
  bool has_minus;

  Location locus;

public:
  // Constructor
  RangePatternBoundLiteral (Literal literal, Location locus,
			    bool has_minus = false)
    : literal (literal), has_minus (has_minus), locus (locus)
  {}

  std::string as_string () const override;

  Location get_locus () const { return locus; }

  void accept_vis (ASTVisitor &vis) override;

protected:
  /* Use covariance to implement clone function as returning this object rather
   * than base */
  RangePatternBoundLiteral *clone_range_pattern_bound_impl () const override
  {
    return new RangePatternBoundLiteral (*this);
  }
};

// Path-based pattern bound
class RangePatternBoundPath : public RangePatternBound
{
  PathInExpression path;

  /* TODO: should this be refactored so that PathInExpression is a subclass of
   * RangePatternBound? */

public:
  RangePatternBoundPath (PathInExpression path) : path (std::move (path)) {}

  std::string as_string () const override { return path.as_string (); }

  Location get_locus () const { return path.get_locus (); }

  void accept_vis (ASTVisitor &vis) override;

  // TODO: this mutable getter seems kinda dodgy
  PathInExpression &get_path () { return path; }
  const PathInExpression &get_path () const { return path; }

protected:
  /* Use covariance to implement clone function as returning this object rather
   * than base */
  RangePatternBoundPath *clone_range_pattern_bound_impl () const override
  {
    return new RangePatternBoundPath (*this);
  }
};

// Qualified path-based pattern bound
class RangePatternBoundQualPath : public RangePatternBound
{
  QualifiedPathInExpression path;

  /* TODO: should this be refactored so that QualifiedPathInExpression is a
   * subclass of RangePatternBound? */

public:
  RangePatternBoundQualPath (QualifiedPathInExpression path)
    : path (std::move (path))
  {}

  std::string as_string () const override { return path.as_string (); }

  Location get_locus () const { return path.get_locus (); }

  void accept_vis (ASTVisitor &vis) override;

  // TODO: this mutable getter seems kinda dodgy
  QualifiedPathInExpression &get_qualified_path () { return path; }
  const QualifiedPathInExpression &get_qualified_path () const { return path; }

protected:
  /* Use covariance to implement clone function as returning this object rather
   * than base */
  RangePatternBoundQualPath *clone_range_pattern_bound_impl () const override
  {
    return new RangePatternBoundQualPath (*this);
  }
};

// AST node for matching within a certain range (range pattern)
class RangePattern : public Pattern
{
  std::unique_ptr<RangePatternBound> lower;
  std::unique_ptr<RangePatternBound> upper;

  bool has_ellipsis_syntax;

  /* location only stored to avoid a dereference - lower pattern should give
   * correct location so maybe change in future */
  Location locus;

public:
  std::string as_string () const override;

  // Constructor
  RangePattern (std::unique_ptr<RangePatternBound> lower,
		std::unique_ptr<RangePatternBound> upper, Location locus,
		bool has_ellipsis_syntax = false)
    : lower (std::move (lower)), upper (std::move (upper)),
      has_ellipsis_syntax (has_ellipsis_syntax), locus (locus)
  {}

  // Copy constructor with clone
  RangePattern (RangePattern const &other)
    : lower (other.lower->clone_range_pattern_bound ()),
      upper (other.upper->clone_range_pattern_bound ()),
      has_ellipsis_syntax (other.has_ellipsis_syntax), locus (other.locus)
  {}

  // Overloaded assignment operator to clone
  RangePattern &operator= (RangePattern const &other)
  {
    lower = other.lower->clone_range_pattern_bound ();
    upper = other.upper->clone_range_pattern_bound ();
    has_ellipsis_syntax = other.has_ellipsis_syntax;
    locus = other.locus;

    return *this;
  }

  // default move semantics
  RangePattern (RangePattern &&other) = default;
  RangePattern &operator= (RangePattern &&other) = default;

  Location get_locus () const { return locus; }
  Location get_locus_slow () const final override { return get_locus (); }

  void accept_vis (ASTVisitor &vis) override;

  // TODO: is this better? or is a "vis_bound" better?
  std::unique_ptr<RangePatternBound> &get_lower_bound ()
  {
    rust_assert (lower != nullptr);
    return lower;
  }

  std::unique_ptr<RangePatternBound> &get_upper_bound ()
  {
    rust_assert (upper != nullptr);
    return upper;
  }

protected:
  /* Use covariance to implement clone function as returning this object rather
   * than base */
  RangePattern *clone_pattern_impl () const override
  {
    return new RangePattern (*this);
  }
};

// AST node for pattern based on dereferencing the pointers given
class ReferencePattern : public Pattern
{
  bool has_two_amps;
  bool is_mut;
  std::unique_ptr<Pattern> pattern;
  Location locus;

public:
  std::string as_string () const override;

  ReferencePattern (std::unique_ptr<Pattern> pattern, bool is_mut_reference,
		    bool ref_has_two_amps, Location locus)
    : has_two_amps (ref_has_two_amps), is_mut (is_mut_reference),
      pattern (std::move (pattern)), locus (locus)
  {}

  // Copy constructor requires clone
  ReferencePattern (ReferencePattern const &other)
    : has_two_amps (other.has_two_amps), is_mut (other.is_mut),
      pattern (other.pattern->clone_pattern ()), locus (other.locus)
  {}

  // Overload assignment operator to clone
  ReferencePattern &operator= (ReferencePattern const &other)
  {
    pattern = other.pattern->clone_pattern ();
    is_mut = other.is_mut;
    has_two_amps = other.has_two_amps;
    locus = other.locus;

    return *this;
  }

  // default move semantics
  ReferencePattern (ReferencePattern &&other) = default;
  ReferencePattern &operator= (ReferencePattern &&other) = default;

  Location get_locus () const { return locus; }
  Location get_locus_slow () const final override { return get_locus (); }

  void accept_vis (ASTVisitor &vis) override;

  // TODO: is this better? Or is a "vis_pattern" better?
  std::unique_ptr<Pattern> &get_referenced_pattern ()
  {
    rust_assert (pattern != nullptr);
    return pattern;
  }

protected:
  /* Use covariance to implement clone function as returning this object rather
   * than base */
  ReferencePattern *clone_pattern_impl () const override
  {
    return new ReferencePattern (*this);
  }
};

#if 0
// aka StructPatternEtCetera; potential element in struct pattern
struct StructPatternEtc
{
private:
  std::vector<Attribute> outer_attrs;

  // should this store location data?

public:
  StructPatternEtc (std::vector<Attribute> outer_attribs)
    : outer_attrs (std::move (outer_attribs))
  {}

  // Creates an empty StructPatternEtc
  static StructPatternEtc create_empty ()
  {
    return StructPatternEtc (std::vector<Attribute> ());
  }
};
#endif

// Base class for a single field in a struct pattern - abstract
class StructPatternField
{
  std::vector<Attribute> outer_attrs;
  Location locus;

public:
  virtual ~StructPatternField () {}

  // Unique pointer custom clone function
  std::unique_ptr<StructPatternField> clone_struct_pattern_field () const
  {
    return std::unique_ptr<StructPatternField> (
      clone_struct_pattern_field_impl ());
  }

  virtual std::string as_string () const;

  Location get_locus () const { return locus; }

  virtual void accept_vis (ASTVisitor &vis) = 0;

  virtual void mark_for_strip () = 0;
  virtual bool is_marked_for_strip () const = 0;

  // TODO: seems kinda dodgy. Think of better way.
  std::vector<Attribute> &get_outer_attrs () { return outer_attrs; }
  const std::vector<Attribute> &get_outer_attrs () const { return outer_attrs; }

protected:
  StructPatternField (std::vector<Attribute> outer_attribs, Location locus)
    : outer_attrs (std::move (outer_attribs)), locus (locus)
  {}

  // Clone function implementation as pure virtual method
  virtual StructPatternField *clone_struct_pattern_field_impl () const = 0;
};

// Tuple pattern single field in a struct pattern
class StructPatternFieldTuplePat : public StructPatternField
{
  TupleIndex index;
  std::unique_ptr<Pattern> tuple_pattern;

public:
  StructPatternFieldTuplePat (TupleIndex index,
			      std::unique_ptr<Pattern> tuple_pattern,
			      std::vector<Attribute> outer_attribs,
			      Location locus)
    : StructPatternField (std::move (outer_attribs), locus), index (index),
      tuple_pattern (std::move (tuple_pattern))
  {}

  // Copy constructor requires clone
  StructPatternFieldTuplePat (StructPatternFieldTuplePat const &other)
    : StructPatternField (other), index (other.index)
  {
    // guard to prevent null dereference (only required if error state)
    if (other.tuple_pattern != nullptr)
      tuple_pattern = other.tuple_pattern->clone_pattern ();
  }

  // Overload assignment operator to perform clone
  StructPatternFieldTuplePat &
  operator= (StructPatternFieldTuplePat const &other)
  {
    StructPatternField::operator= (other);
    index = other.index;
    // outer_attrs = other.outer_attrs;

    // guard to prevent null dereference (only required if error state)
    if (other.tuple_pattern != nullptr)
      tuple_pattern = other.tuple_pattern->clone_pattern ();
    else
      tuple_pattern = nullptr;

    return *this;
  }

  // default move semantics
  StructPatternFieldTuplePat (StructPatternFieldTuplePat &&other) = default;
  StructPatternFieldTuplePat &operator= (StructPatternFieldTuplePat &&other)
    = default;

  std::string as_string () const override;

  void accept_vis (ASTVisitor &vis) override;

  // based on idea of tuple pattern no longer existing
  void mark_for_strip () override { tuple_pattern = nullptr; }
  bool is_marked_for_strip () const override
  {
    return tuple_pattern == nullptr;
  }

  // TODO: is this better? Or is a "vis_pattern" better?
  std::unique_ptr<Pattern> &get_index_pattern ()
  {
    rust_assert (tuple_pattern != nullptr);
    return tuple_pattern;
  }

protected:
  /* Use covariance to implement clone function as returning this object rather
   * than base */
  StructPatternFieldTuplePat *clone_struct_pattern_field_impl () const override
  {
    return new StructPatternFieldTuplePat (*this);
  }
};

// Identifier pattern single field in a struct pattern
class StructPatternFieldIdentPat : public StructPatternField
{
  Identifier ident;
  std::unique_ptr<Pattern> ident_pattern;

public:
  StructPatternFieldIdentPat (Identifier ident,
			      std::unique_ptr<Pattern> ident_pattern,
			      std::vector<Attribute> outer_attrs,
			      Location locus)
    : StructPatternField (std::move (outer_attrs), locus),
      ident (std::move (ident)), ident_pattern (std::move (ident_pattern))
  {}

  // Copy constructor requires clone
  StructPatternFieldIdentPat (StructPatternFieldIdentPat const &other)
    : StructPatternField (other), ident (other.ident)
  {
    // guard to prevent null dereference (only required if error state)
    if (other.ident_pattern != nullptr)
      ident_pattern = other.ident_pattern->clone_pattern ();
  }

  // Overload assignment operator to clone
  StructPatternFieldIdentPat &
  operator= (StructPatternFieldIdentPat const &other)
  {
    StructPatternField::operator= (other);
    ident = other.ident;
    // outer_attrs = other.outer_attrs;

    // guard to prevent null dereference (only required if error state)
    if (other.ident_pattern != nullptr)
      ident_pattern = other.ident_pattern->clone_pattern ();
    else
      ident_pattern = nullptr;

    return *this;
  }

  // default move semantics
  StructPatternFieldIdentPat (StructPatternFieldIdentPat &&other) = default;
  StructPatternFieldIdentPat &operator= (StructPatternFieldIdentPat &&other)
    = default;

  std::string as_string () const override;

  void accept_vis (ASTVisitor &vis) override;

  // based on idea of identifier pattern no longer existing
  void mark_for_strip () override { ident_pattern = nullptr; }
  bool is_marked_for_strip () const override
  {
    return ident_pattern == nullptr;
  }

  // TODO: is this better? Or is a "vis_pattern" better?
  std::unique_ptr<Pattern> &get_ident_pattern ()
  {
    rust_assert (ident_pattern != nullptr);
    return ident_pattern;
  }

protected:
  /* Use covariance to implement clone function as returning this object rather
   * than base */
  StructPatternFieldIdentPat *clone_struct_pattern_field_impl () const override
  {
    return new StructPatternFieldIdentPat (*this);
  }
};

// Identifier only (with no pattern) single field in a struct pattern
class StructPatternFieldIdent : public StructPatternField
{
  bool has_ref;
  bool has_mut;
  Identifier ident;

public:
  StructPatternFieldIdent (Identifier ident, bool is_ref, bool is_mut,
			   std::vector<Attribute> outer_attrs, Location locus)
    : StructPatternField (std::move (outer_attrs), locus), has_ref (is_ref),
      has_mut (is_mut), ident (std::move (ident))
  {}

  std::string as_string () const override;

  void accept_vis (ASTVisitor &vis) override;

  // based on idea of identifier no longer existing
  void mark_for_strip () override { ident = {}; }
  bool is_marked_for_strip () const override { return ident.empty (); }

protected:
  /* Use covariance to implement clone function as returning this object rather
   * than base */
  StructPatternFieldIdent *clone_struct_pattern_field_impl () const override
  {
    return new StructPatternFieldIdent (*this);
  }
};

// Elements of a struct pattern
struct StructPatternElements
{
private:
  // bool has_struct_pattern_fields;
  std::vector<std::unique_ptr<StructPatternField> > fields;

  bool has_struct_pattern_etc;
  std::vector<Attribute> struct_pattern_etc_attrs;
  // StructPatternEtc etc;

  // must have at least one of the two and maybe both

  // should this store location data?

public:
  // Returns whether there are any struct pattern fields
  bool has_struct_pattern_fields () const { return !fields.empty (); }

  /* Returns whether the struct pattern elements is entirely empty (no fields,
   * no etc). */
  bool is_empty () const
  {
    return !has_struct_pattern_fields () && !has_struct_pattern_etc;
  }

  bool has_etc () const { return has_struct_pattern_etc; }

  // Constructor for StructPatternElements with both (potentially)
  StructPatternElements (
    std::vector<std::unique_ptr<StructPatternField> > fields,
    std::vector<Attribute> etc_attrs)
    : fields (std::move (fields)), has_struct_pattern_etc (true),
      struct_pattern_etc_attrs (std::move (etc_attrs))
  {}

  // Constructor for StructPatternElements with no StructPatternEtc
  StructPatternElements (
    std::vector<std::unique_ptr<StructPatternField> > fields)
    : fields (std::move (fields)), has_struct_pattern_etc (false),
      struct_pattern_etc_attrs ()
  {}

  // Copy constructor with vector clone
  StructPatternElements (StructPatternElements const &other)
    : has_struct_pattern_etc (other.has_struct_pattern_etc),
      struct_pattern_etc_attrs (other.struct_pattern_etc_attrs)
  {
    fields.reserve (other.fields.size ());
    for (const auto &e : other.fields)
      fields.push_back (e->clone_struct_pattern_field ());
  }

  // Overloaded assignment operator with vector clone
  StructPatternElements &operator= (StructPatternElements const &other)
  {
    struct_pattern_etc_attrs = other.struct_pattern_etc_attrs;
    has_struct_pattern_etc = other.has_struct_pattern_etc;

    fields.reserve (other.fields.size ());
    for (const auto &e : other.fields)
      fields.push_back (e->clone_struct_pattern_field ());

    return *this;
  }

  // move constructors
  StructPatternElements (StructPatternElements &&other) = default;
  StructPatternElements &operator= (StructPatternElements &&other) = default;

  // Creates an empty StructPatternElements
  static StructPatternElements create_empty ()
  {
    return StructPatternElements (
      std::vector<std::unique_ptr<StructPatternField> > ());
  }

  std::string as_string () const;

  // TODO: seems kinda dodgy. Think of better way.
  std::vector<std::unique_ptr<StructPatternField> > &
  get_struct_pattern_fields ()
  {
    return fields;
  }
  const std::vector<std::unique_ptr<StructPatternField> > &
  get_struct_pattern_fields () const
  {
    return fields;
  }

  std::vector<Attribute> &get_etc_outer_attrs ()
  {
    return struct_pattern_etc_attrs;
  }
  const std::vector<Attribute> &get_etc_outer_attrs () const
  {
    return struct_pattern_etc_attrs;
  }

  void strip_etc ()
  {
    has_struct_pattern_etc = false;
    struct_pattern_etc_attrs.clear ();
    struct_pattern_etc_attrs.shrink_to_fit ();
  }
};

// Struct pattern AST node representation
class StructPattern : public Pattern
{
  PathInExpression path;

  // bool has_struct_pattern_elements;
  StructPatternElements elems;

  // TODO: should this store location data? Accessor uses path location data.

public:
  std::string as_string () const override;

  // Constructs a struct pattern from specified StructPatternElements
  StructPattern (PathInExpression struct_path,
		 StructPatternElements elems
		 = StructPatternElements::create_empty ())
    : path (std::move (struct_path)), elems (std::move (elems))
  {}

  /* TODO: constructor to construct via elements included in
   * StructPatternElements */

  /* Returns whether struct pattern has any struct pattern elements (if not, it
   * is empty). */
  bool has_struct_pattern_elems () const { return !elems.is_empty (); }

  Location get_locus () const { return path.get_locus (); }
  Location get_locus_slow () const final override { return get_locus (); }

  void accept_vis (ASTVisitor &vis) override;

  // TODO: seems kinda dodgy. Think of better way.
  StructPatternElements &get_struct_pattern_elems () { return elems; }
  const StructPatternElements &get_struct_pattern_elems () const
  {
    return elems;
  }

  PathInExpression &get_path () { return path; }
  const PathInExpression &get_path () const { return path; }

protected:
  /* Use covariance to implement clone function as returning this object rather
   * than base */
  StructPattern *clone_pattern_impl () const override
  {
    return new StructPattern (*this);
  }
};

// Base abstract class for patterns used in TupleStructPattern
class TupleStructItems
{
public:
  virtual ~TupleStructItems () {}

  // TODO: should this store location data?

  // Unique pointer custom clone function
  std::unique_ptr<TupleStructItems> clone_tuple_struct_items () const
  {
    return std::unique_ptr<TupleStructItems> (clone_tuple_struct_items_impl ());
  }

  virtual std::string as_string () const = 0;

  virtual void accept_vis (ASTVisitor &vis) = 0;

protected:
  // pure virtual clone implementation
  virtual TupleStructItems *clone_tuple_struct_items_impl () const = 0;
};

// Class for non-ranged tuple struct pattern patterns
class TupleStructItemsNoRange : public TupleStructItems
{
  std::vector<std::unique_ptr<Pattern> > patterns;

public:
  TupleStructItemsNoRange (std::vector<std::unique_ptr<Pattern> > patterns)
    : patterns (std::move (patterns))
  {}

  // Copy constructor with vector clone
  TupleStructItemsNoRange (TupleStructItemsNoRange const &other)
  {
    patterns.reserve (other.patterns.size ());
    for (const auto &e : other.patterns)
      patterns.push_back (e->clone_pattern ());
  }

  // Overloaded assignment operator with vector clone
  TupleStructItemsNoRange &operator= (TupleStructItemsNoRange const &other)
  {
    patterns.reserve (other.patterns.size ());
    for (const auto &e : other.patterns)
      patterns.push_back (e->clone_pattern ());

    return *this;
  }

  // move constructors
  TupleStructItemsNoRange (TupleStructItemsNoRange &&other) = default;
  TupleStructItemsNoRange &operator= (TupleStructItemsNoRange &&other)
    = default;

  std::string as_string () const override;

  void accept_vis (ASTVisitor &vis) override;

  // TODO: seems kinda dodgy. Think of better way.
  std::vector<std::unique_ptr<Pattern> > &get_patterns () { return patterns; }
  const std::vector<std::unique_ptr<Pattern> > &get_patterns () const
  {
    return patterns;
  }

protected:
  /* Use covariance to implement clone function as returning this object rather
   * than base */
  TupleStructItemsNoRange *clone_tuple_struct_items_impl () const override
  {
    return new TupleStructItemsNoRange (*this);
  }
};

// Class for ranged tuple struct pattern patterns
class TupleStructItemsRange : public TupleStructItems
{
  std::vector<std::unique_ptr<Pattern> > lower_patterns;
  std::vector<std::unique_ptr<Pattern> > upper_patterns;

public:
  TupleStructItemsRange (std::vector<std::unique_ptr<Pattern> > lower_patterns,
			 std::vector<std::unique_ptr<Pattern> > upper_patterns)
    : lower_patterns (std::move (lower_patterns)),
      upper_patterns (std::move (upper_patterns))
  {}

  // Copy constructor with vector clone
  TupleStructItemsRange (TupleStructItemsRange const &other)
  {
    lower_patterns.reserve (other.lower_patterns.size ());
    for (const auto &e : other.lower_patterns)
      lower_patterns.push_back (e->clone_pattern ());

    upper_patterns.reserve (other.upper_patterns.size ());
    for (const auto &e : other.upper_patterns)
      upper_patterns.push_back (e->clone_pattern ());
  }

  // Overloaded assignment operator to clone
  TupleStructItemsRange &operator= (TupleStructItemsRange const &other)
  {
    lower_patterns.reserve (other.lower_patterns.size ());
    for (const auto &e : other.lower_patterns)
      lower_patterns.push_back (e->clone_pattern ());

    upper_patterns.reserve (other.upper_patterns.size ());
    for (const auto &e : other.upper_patterns)
      upper_patterns.push_back (e->clone_pattern ());

    return *this;
  }

  // move constructors
  TupleStructItemsRange (TupleStructItemsRange &&other) = default;
  TupleStructItemsRange &operator= (TupleStructItemsRange &&other) = default;

  std::string as_string () const override;

  void accept_vis (ASTVisitor &vis) override;

  // TODO: seems kinda dodgy. Think of better way.
  std::vector<std::unique_ptr<Pattern> > &get_lower_patterns ()
  {
    return lower_patterns;
  }
  const std::vector<std::unique_ptr<Pattern> > &get_lower_patterns () const
  {
    return lower_patterns;
  }

  // TODO: seems kinda dodgy. Think of better way.
  std::vector<std::unique_ptr<Pattern> > &get_upper_patterns ()
  {
    return upper_patterns;
  }
  const std::vector<std::unique_ptr<Pattern> > &get_upper_patterns () const
  {
    return upper_patterns;
  }

protected:
  /* Use covariance to implement clone function as returning this object rather
   * than base */
  TupleStructItemsRange *clone_tuple_struct_items_impl () const override
  {
    return new TupleStructItemsRange (*this);
  }
};

// AST node representing a tuple struct pattern
class TupleStructPattern : public Pattern
{
  PathInExpression path;
  std::unique_ptr<TupleStructItems> items;

  /* TOOD: should this store location data? current accessor uses path location
   * data */

public:
  std::string as_string () const override;

  // Returns whether the pattern has tuple struct items.
  bool has_items () const { return items != nullptr; }

  TupleStructPattern (PathInExpression tuple_struct_path,
		      std::unique_ptr<TupleStructItems> items)
    : path (std::move (tuple_struct_path)), items (std::move (items))
  {}

  // Copy constructor required to clone
  TupleStructPattern (TupleStructPattern const &other) : path (other.path)
  {
    // guard to protect from null dereference
    if (other.items != nullptr)
      items = other.items->clone_tuple_struct_items ();
  }

  // Operator overload assignment operator to clone
  TupleStructPattern &operator= (TupleStructPattern const &other)
  {
    path = other.path;

    // guard to protect from null dereference
    if (other.items != nullptr)
      items = other.items->clone_tuple_struct_items ();
    else
      items = nullptr;

    return *this;
  }

  // move constructors
  TupleStructPattern (TupleStructPattern &&other) = default;
  TupleStructPattern &operator= (TupleStructPattern &&other) = default;

  Location get_locus () const { return path.get_locus (); }
  Location get_locus_slow () const final override { return get_locus (); }

  void accept_vis (ASTVisitor &vis) override;

  // TODO: seems kinda dodgy. Think of better way.
  std::unique_ptr<TupleStructItems> &get_items ()
  {
    rust_assert (has_items ());
    return items;
  }

  PathInExpression &get_path () { return path; }
  const PathInExpression &get_path () const { return path; }

protected:
  /* Use covariance to implement clone function as returning this object rather
   * than base */
  TupleStructPattern *clone_pattern_impl () const override
  {
    return new TupleStructPattern (*this);
  }
};

// Base abstract class representing TuplePattern patterns
class TuplePatternItems
{
public:
  virtual ~TuplePatternItems () {}

  // TODO: should this store location data?

  // Unique pointer custom clone function
  std::unique_ptr<TuplePatternItems> clone_tuple_pattern_items () const
  {
    return std::unique_ptr<TuplePatternItems> (
      clone_tuple_pattern_items_impl ());
  }

  virtual std::string as_string () const = 0;

  virtual void accept_vis (ASTVisitor &vis) = 0;

protected:
  // pure virtual clone implementation
  virtual TuplePatternItems *clone_tuple_pattern_items_impl () const = 0;
};

// Class representing TuplePattern patterns where there is only a single pattern
/*class TuplePatternItemsSingle : public TuplePatternItems {
    // Pattern pattern;
    std::unique_ptr<Pattern> pattern;

  public:
    TuplePatternItemsSingle(Pattern* pattern) : pattern(pattern) {}

    // Copy constructor uses clone
    TuplePatternItemsSingle(TuplePatternItemsSingle const& other) :
      pattern(other.pattern->clone_pattern()) {}

    // Destructor - define here if required

    // Overload assignment operator to clone
    TuplePatternItemsSingle& operator=(TuplePatternItemsSingle const& other) {
	pattern = other.pattern->clone_pattern();

	return *this;
    }

    // move constructors
    TuplePatternItemsSingle(TuplePatternItemsSingle&& other) = default;
    TuplePatternItemsSingle& operator=(TuplePatternItemsSingle&& other) =
default;

  protected:
    // Use covariance to implement clone function as returning this object
rather than base virtual TuplePatternItemsSingle*
clone_tuple_pattern_items_impl() const override { return new
TuplePatternItemsSingle(*this);
    }
};*/
// removed in favour of single-element TuplePatternItemsMultiple

// Class representing TuplePattern patterns where there are multiple patterns
class TuplePatternItemsMultiple : public TuplePatternItems
{
  std::vector<std::unique_ptr<Pattern> > patterns;

public:
  TuplePatternItemsMultiple (std::vector<std::unique_ptr<Pattern> > patterns)
    : patterns (std::move (patterns))
  {}

  // Copy constructor with vector clone
  TuplePatternItemsMultiple (TuplePatternItemsMultiple const &other)
  {
    patterns.reserve (other.patterns.size ());
    for (const auto &e : other.patterns)
      patterns.push_back (e->clone_pattern ());
  }

  // Overloaded assignment operator to vector clone
  TuplePatternItemsMultiple &operator= (TuplePatternItemsMultiple const &other)
  {
    patterns.reserve (other.patterns.size ());
    for (const auto &e : other.patterns)
      patterns.push_back (e->clone_pattern ());

    return *this;
  }

  // move constructors
  TuplePatternItemsMultiple (TuplePatternItemsMultiple &&other) = default;
  TuplePatternItemsMultiple &operator= (TuplePatternItemsMultiple &&other)
    = default;

  std::string as_string () const override;

  void accept_vis (ASTVisitor &vis) override;

  // TODO: seems kinda dodgy. Think of better way.
  std::vector<std::unique_ptr<Pattern> > &get_patterns () { return patterns; }
  const std::vector<std::unique_ptr<Pattern> > &get_patterns () const
  {
    return patterns;
  }

protected:
  /* Use covariance to implement clone function as returning this object rather
   * than base */
  TuplePatternItemsMultiple *clone_tuple_pattern_items_impl () const override
  {
    return new TuplePatternItemsMultiple (*this);
  }
};

// Class representing TuplePattern patterns where there are a range of patterns
class TuplePatternItemsRanged : public TuplePatternItems
{
  std::vector<std::unique_ptr<Pattern> > lower_patterns;
  std::vector<std::unique_ptr<Pattern> > upper_patterns;

public:
  TuplePatternItemsRanged (
    std::vector<std::unique_ptr<Pattern> > lower_patterns,
    std::vector<std::unique_ptr<Pattern> > upper_patterns)
    : lower_patterns (std::move (lower_patterns)),
      upper_patterns (std::move (upper_patterns))
  {}

  // Copy constructor with vector clone
  TuplePatternItemsRanged (TuplePatternItemsRanged const &other)
  {
    lower_patterns.reserve (other.lower_patterns.size ());
    for (const auto &e : other.lower_patterns)
      lower_patterns.push_back (e->clone_pattern ());

    upper_patterns.reserve (other.upper_patterns.size ());
    for (const auto &e : other.upper_patterns)
      upper_patterns.push_back (e->clone_pattern ());
  }

  // Overloaded assignment operator to clone
  TuplePatternItemsRanged &operator= (TuplePatternItemsRanged const &other)
  {
    lower_patterns.reserve (other.lower_patterns.size ());
    for (const auto &e : other.lower_patterns)
      lower_patterns.push_back (e->clone_pattern ());

    upper_patterns.reserve (other.upper_patterns.size ());
    for (const auto &e : other.upper_patterns)
      upper_patterns.push_back (e->clone_pattern ());

    return *this;
  }

  // move constructors
  TuplePatternItemsRanged (TuplePatternItemsRanged &&other) = default;
  TuplePatternItemsRanged &operator= (TuplePatternItemsRanged &&other)
    = default;

  std::string as_string () const override;

  void accept_vis (ASTVisitor &vis) override;

  // TODO: seems kinda dodgy. Think of better way.
  std::vector<std::unique_ptr<Pattern> > &get_lower_patterns ()
  {
    return lower_patterns;
  }
  const std::vector<std::unique_ptr<Pattern> > &get_lower_patterns () const
  {
    return lower_patterns;
  }

  // TODO: seems kinda dodgy. Think of better way.
  std::vector<std::unique_ptr<Pattern> > &get_upper_patterns ()
  {
    return upper_patterns;
  }
  const std::vector<std::unique_ptr<Pattern> > &get_upper_patterns () const
  {
    return upper_patterns;
  }

protected:
  /* Use covariance to implement clone function as returning this object rather
   * than base */
  TuplePatternItemsRanged *clone_tuple_pattern_items_impl () const override
  {
    return new TuplePatternItemsRanged (*this);
  }
};

// AST node representing a tuple pattern
class TuplePattern : public Pattern
{
  // bool has_tuple_pattern_items;
  std::unique_ptr<TuplePatternItems> items;
  Location locus;

public:
  std::string as_string () const override;

  // Returns true if the tuple pattern has items
  bool has_tuple_pattern_items () const { return items != nullptr; }

  TuplePattern (std::unique_ptr<TuplePatternItems> items, Location locus)
    : items (std::move (items)), locus (locus)
  {}

  // Copy constructor requires clone
  TuplePattern (TuplePattern const &other) : locus (other.locus)
  {
    // guard to prevent null dereference
    if (other.items != nullptr)
      items = other.items->clone_tuple_pattern_items ();
  }

  // Overload assignment operator to clone
  TuplePattern &operator= (TuplePattern const &other)
  {
    locus = other.locus;

    // guard to prevent null dereference
    if (other.items != nullptr)
      items = other.items->clone_tuple_pattern_items ();
    else
      items = nullptr;

    return *this;
  }

  Location get_locus () const { return locus; }
  Location get_locus_slow () const final override { return get_locus (); }

  void accept_vis (ASTVisitor &vis) override;

  // TODO: seems kinda dodgy. Think of better way.
  std::unique_ptr<TuplePatternItems> &get_items ()
  {
    rust_assert (has_tuple_pattern_items ());
    return items;
  }

protected:
  /* Use covariance to implement clone function as returning this object rather
   * than base */
  TuplePattern *clone_pattern_impl () const override
  {
    return new TuplePattern (*this);
  }
};

// AST node representing a pattern in parentheses, used to control precedence
class GroupedPattern : public Pattern
{
  std::unique_ptr<Pattern> pattern_in_parens;
  Location locus;

public:
  std::string as_string () const override
  {
    return "(" + pattern_in_parens->as_string () + ")";
  }

  GroupedPattern (std::unique_ptr<Pattern> pattern_in_parens, Location locus)
    : pattern_in_parens (std::move (pattern_in_parens)), locus (locus)
  {}

  // Copy constructor uses clone
  GroupedPattern (GroupedPattern const &other)
    : pattern_in_parens (other.pattern_in_parens->clone_pattern ()),
      locus (other.locus)
  {}

  // Overload assignment operator to clone
  GroupedPattern &operator= (GroupedPattern const &other)
  {
    pattern_in_parens = other.pattern_in_parens->clone_pattern ();
    locus = other.locus;

    return *this;
  }

  // default move semantics
  GroupedPattern (GroupedPattern &&other) = default;
  GroupedPattern &operator= (GroupedPattern &&other) = default;

  Location get_locus () const { return locus; }
  Location get_locus_slow () const final override { return get_locus (); }

  void accept_vis (ASTVisitor &vis) override;

  // TODO: seems kinda dodgy. Think of better way.
  std::unique_ptr<Pattern> &get_pattern_in_parens ()
  {
    rust_assert (pattern_in_parens != nullptr);
    return pattern_in_parens;
  }

protected:
  /* Use covariance to implement clone function as returning this object rather
   * than base */
  GroupedPattern *clone_pattern_impl () const override
  {
    return new GroupedPattern (*this);
  }
};

// AST node representing patterns that can match slices and arrays
class SlicePattern : public Pattern
{
  std::vector<std::unique_ptr<Pattern> > items;
  Location locus;

public:
  std::string as_string () const override;

  SlicePattern (std::vector<std::unique_ptr<Pattern> > items, Location locus)
    : items (std::move (items)), locus (locus)
  {}

  // Copy constructor with vector clone
  SlicePattern (SlicePattern const &other) : locus (other.locus)
  {
    items.reserve (other.items.size ());
    for (const auto &e : other.items)
      items.push_back (e->clone_pattern ());
  }

  // Overloaded assignment operator to vector clone
  SlicePattern &operator= (SlicePattern const &other)
  {
    locus = other.locus;

    items.reserve (other.items.size ());
    for (const auto &e : other.items)
      items.push_back (e->clone_pattern ());

    return *this;
  }

  // move constructors
  SlicePattern (SlicePattern &&other) = default;
  SlicePattern &operator= (SlicePattern &&other) = default;

  Location get_locus () const { return locus; }
  Location get_locus_slow () const final override { return get_locus (); }

  void accept_vis (ASTVisitor &vis) override;

  // TODO: seems kinda dodgy. Think of better way.
  std::vector<std::unique_ptr<Pattern> > &get_items () { return items; }
  const std::vector<std::unique_ptr<Pattern> > &get_items () const
  {
    return items;
  }

protected:
  /* Use covariance to implement clone function as returning this object rather
   * than base */
  SlicePattern *clone_pattern_impl () const override
  {
    return new SlicePattern (*this);
  }
};

// Moved definition to rust-path.h
class PathPattern;

// Forward decls for paths (defined in rust-path.h)
class PathInExpression;
class QualifiedPathInExpression;

// Replaced with forward decl - defined in rust-macro.h
class MacroInvocation;
} // namespace AST
} // namespace Rust

#endif
