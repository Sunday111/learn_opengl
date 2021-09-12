#pragma once

template <typename Member>
struct ClassMemberTraits {};

template <typename InClass, typename InMember>
struct ClassMemberTraits<InMember InClass::*> {
  using Class = InClass;
  using Member = InMember;
};