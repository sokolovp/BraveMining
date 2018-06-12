// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGContainerFragmentBuilder_h
#define NGContainerFragmentBuilder_h

#include "base/memory/scoped_refptr.h"
#include "core/CoreExport.h"
#include "core/layout/ng/geometry/ng_bfc_offset.h"
#include "core/layout/ng/geometry/ng_logical_size.h"
#include "core/layout/ng/geometry/ng_margin_strut.h"
#include "core/layout/ng/ng_base_fragment_builder.h"
#include "core/layout/ng/ng_out_of_flow_positioned_descendant.h"
#include "platform/text/TextDirection.h"
#include "platform/text/WritingMode.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class ComputedStyle;
class NGExclusionSpace;
class NGLayoutResult;
class NGPhysicalFragment;
struct NGUnpositionedFloat;

class CORE_EXPORT NGContainerFragmentBuilder : public NGBaseFragmentBuilder {
  STACK_ALLOCATED();

 public:
  ~NGContainerFragmentBuilder() override;

  LayoutUnit InlineSize() const { return inline_size_; }
  NGContainerFragmentBuilder& SetInlineSize(LayoutUnit);
  void SetBlockSize(LayoutUnit block_size) { block_size_ = block_size; }

  virtual NGLogicalSize Size() const = 0;

  // The NGBfcOffset is where this fragment was positioned within the BFC. If
  // it is not set, this fragment may be placed anywhere within the BFC.
  const WTF::Optional<NGBfcOffset>& BfcOffset() const { return bfc_offset_; }
  NGContainerFragmentBuilder& SetBfcOffset(const NGBfcOffset&);

  NGContainerFragmentBuilder& SetEndMarginStrut(const NGMarginStrut&);

  NGContainerFragmentBuilder& SetExclusionSpace(
      std::unique_ptr<const NGExclusionSpace> exclusion_space);

  NGContainerFragmentBuilder& SwapUnpositionedFloats(
      Vector<scoped_refptr<NGUnpositionedFloat>>*);

  const NGBlockNode& UnpositionedListMarker() const {
    return unpositioned_list_marker_;
  }
  NGContainerFragmentBuilder& SetUnpositionedListMarker(const NGBlockNode&);

  virtual NGContainerFragmentBuilder& AddChild(scoped_refptr<NGLayoutResult>,
                                               const NGLogicalOffset&);

  // This version of AddChild will not propagate floats/out_of_flow.
  // Use the AddChild(NGLayoutResult) variant if NGLayoutResult is available.
  virtual NGContainerFragmentBuilder& AddChild(
      scoped_refptr<NGPhysicalFragment>,
      const NGLogicalOffset&);

  const Vector<scoped_refptr<NGPhysicalFragment>>& Children() const {
    return children_;
  }

  // Builder has non-trivial out-of-flow descendant methods.
  // These methods are building blocks for implementation of
  // out-of-flow descendants by layout algorithms.
  //
  // They are intended to be used by layout algorithm like this:
  //
  // Part 1: layout algorithm positions in-flow children.
  //   out-of-flow children, and out-of-flow descendants of fragments
  //   are stored inside builder.
  //
  // for (child : children)
  //   if (child->position == (Absolute or Fixed))
  //     builder->AddOutOfFlowChildCandidate(child);
  //   else
  //     fragment = child->Layout()
  //     builder->AddChild(fragment)
  // end
  //
  // builder->SetSize
  //
  // Part 2: Out-of-flow layout part positions out-of-flow descendants.
  //
  // NGOutOfFlowLayoutPart(container_style, builder).Run();
  //
  // See layout part for builder interaction.
  //
  // @param direction: default candidate direction is builder's direction.
  // Pass in direction if candidates direction does not match.
  NGContainerFragmentBuilder& AddOutOfFlowChildCandidate(
      NGBlockNode,
      const NGLogicalOffset& child_offset);

  // Inline candidates are laid out line-relative, not fragment-relative.
  NGContainerFragmentBuilder& AddInlineOutOfFlowChildCandidate(
      NGBlockNode,
      const NGLogicalOffset& child_line_offset,
      TextDirection line_direction,
      LayoutObject* inline_container);

  NGContainerFragmentBuilder& AddOutOfFlowDescendant(
      NGOutOfFlowPositionedDescendant);

  void GetAndClearOutOfFlowDescendantCandidates(
      Vector<NGOutOfFlowPositionedDescendant>* descendant_candidates,
      const LayoutObject* container);

  NGContainerFragmentBuilder& SetIsPushedByFloats() {
    is_pushed_by_floats_ = true;
    return *this;
  }

#ifndef NDEBUG
  String ToString() const;
#endif

 protected:
  // An out-of-flow positioned-candidate is a temporary data structure used
  // within the NGFragmentBuilder.
  //
  // A positioned-candidate can be:
  // 1. A direct out-of-flow positioned child. The child_offset is (0,0).
  // 2. A fragment containing an out-of-flow positioned-descendant. The
  //    child_offset in this case is the containing fragment's offset.
  //
  // The child_offset is stored as a NGLogicalOffset as the physical offset
  // cannot be computed until we know the current fragment's size.
  //
  // When returning the positioned-candidates (from
  // GetAndClearOutOfFlowDescendantCandidates), the NGFragmentBuilder will
  // convert the positioned-candidate to a positioned-descendant using the
  // physical size the fragment builder.
  struct NGOutOfFlowPositionedCandidate {
    NGOutOfFlowPositionedDescendant descendant;
    NGLogicalOffset child_offset;  // Logical offset of child's top left vertex.
    bool is_line_relative;  // True if offset is relative to line, not fragment.
    TextDirection line_direction;

    NGOutOfFlowPositionedCandidate(
        NGOutOfFlowPositionedDescendant descendant_arg,
        NGLogicalOffset child_offset_arg)
        : descendant(descendant_arg),
          child_offset(child_offset_arg),
          is_line_relative(false) {}

    NGOutOfFlowPositionedCandidate(
        NGOutOfFlowPositionedDescendant descendant_arg,
        NGLogicalOffset child_offset_arg,
        TextDirection line_direction_arg)
        : descendant(descendant_arg),
          child_offset(child_offset_arg),
          is_line_relative(true),
          line_direction(line_direction_arg) {}
  };

  NGContainerFragmentBuilder(scoped_refptr<const ComputedStyle>,
                             WritingMode,
                             TextDirection);

  LayoutUnit inline_size_;
  LayoutUnit block_size_;

  WTF::Optional<NGBfcOffset> bfc_offset_;
  NGMarginStrut end_margin_strut_;
  std::unique_ptr<const NGExclusionSpace> exclusion_space_;

  // Floats that need to be positioned by the next in-flow fragment that can
  // determine its block position in space.
  Vector<scoped_refptr<NGUnpositionedFloat>> unpositioned_floats_;

  Vector<NGOutOfFlowPositionedCandidate> oof_positioned_candidates_;
  Vector<NGOutOfFlowPositionedDescendant> oof_positioned_descendants_;

  NGBlockNode unpositioned_list_marker_;

  Vector<scoped_refptr<NGPhysicalFragment>> children_;
  Vector<NGLogicalOffset> offsets_;

  bool has_last_resort_break_ = false;

  bool is_pushed_by_floats_ = false;
};

}  // namespace blink

#endif  // NGContainerFragmentBuilder