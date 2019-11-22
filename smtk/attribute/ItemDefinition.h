//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
// .NAME ItemDefinition.h - the definition of a value of an attribute definition.
// .SECTION Description
// ItemDefinition is meant to store definitions of values that can be
// stored inside of an attribute. Derived classes give specific
// types of items.
// .SECTION See Also

#ifndef __smtk_attribute_ItemDefinition_h
#define __smtk_attribute_ItemDefinition_h

#include "smtk/CoreExports.h"
#include "smtk/PublicPointerDefs.h"
#include "smtk/SharedFromThis.h" // For smtkTypeMacro.
#include "smtk/attribute/Item.h" // For Item Types.

#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace smtk
{
namespace attribute
{
class Attribute;
class Item;
class GroupItemDefinition;
class SMTKCORE_EXPORT ItemDefinition
{
  friend class smtk::attribute::Definition;
  friend class smtk::attribute::GroupItemDefinition;
  friend class smtk::attribute::ValueItemDefinition;

public:
  smtkTypeMacroBase(smtk::attribute::ItemDefinition);
  // Temp structure used for copying definitions
  struct CopyInfo
  {
    // Reference to resource that is getting modified ("to")
    const smtk::attribute::Resource& ToResource;
    // List of ValueItemDefinitions that reference expressions not currently in this resource
    std::queue<std::pair<std::string, smtk::attribute::ItemDefinitionPtr> > UnresolvedExpItems;
    CopyInfo(const smtk::attribute::ResourcePtr resource)
      : ToResource(*resource)
    {
    }
  };

  virtual ~ItemDefinition();
  // The name used to access the item - this name is unique w/r to the attribute
  // or parent item
  const std::string& name() const { return m_name; }

  virtual Item::Type type() const = 0;
  // The label is what can be displayed in an application.  Unlike the type
  // which is constant w/r to the definition, an application can change the label
  const std::string& label() const { return (m_label != "" ? m_label : m_name); }

  void setLabel(const std::string& newLabel) { m_label = newLabel; }

  int version() const { return m_version; }
  void setVersion(int myVersion) { m_version = myVersion; }

  bool isOptional() const { return m_isOptional; }

  void setIsOptional(bool isOptionalValue) { m_isOptional = isOptionalValue; }

  // This only comes into play if the item is optional
  bool isEnabledByDefault() const { return m_isEnabledByDefault; }

  void setIsEnabledByDefault(bool isEnabledByDefaultValue)
  {
    m_isEnabledByDefault = isEnabledByDefaultValue;
  }

  enum class CategoryCheckMode
  {
    Any = 0, //!< Check passes if any of the definition's categories are found (Default)
    All = 1  //!< Check passes if all of the definition's categories are found
  };

  void setCategoryCheckMode(CategoryCheckMode mode) { m_categoryCheckMode = mode; }

  CategoryCheckMode categoryCheckMode() const { return m_categoryCheckMode; }

  /// @{
  /// \brief Based on it's categoryCheckMode, checks to see if the definition's categories passes with respects to the input category/categories.
  ///
  /// If the mode is Any then if at least one of its categories is in the input then it passes, else if the mode is All then all of the
  /// the definition's categories must be contained.  If the input set is empty then the check will always pass, else if the definition
  /// has no categories then the check will always fail.
  bool passCategoryCheck(const std::string& category) const;
  bool passCategoryCheck(const std::set<std::string>& categories) const;
  /// @}

  std::size_t numberOfCategories() const { return m_categories.size(); }

  ///\brief Returns the categories (both explicitly assigned and inherited) associated to the Definition
  const std::set<std::string>& categories() const { return m_categories; }

  bool isMemberOf(const std::string& category) const
  {
    return (m_categories.find(category) != m_categories.end());
  }

  bool isMemberOf(const std::vector<std::string>& categories) const;

  ///\brief Indicates if the Definition can inherit categories based on it's
  /// parent Definition or its owning Attribute Definition.  The default is true.
  bool isOkToInherit() const { return m_isOkToInherit; }

  void setIsOkToInherit(bool isOkToInheritValue) { m_isOkToInherit = isOkToInheritValue; }

  ///\brief Returns the categories explicitly assigned to the Definition
  const std::set<std::string>& localCategories() const { return m_localCategories; }

  virtual void addLocalCategory(const std::string& category);

  virtual void removeLocalCategory(const std::string& category);

  //Get the item definition's advance level:
  //if mode is 1 then the write access level is returned;
  //else the read access level is returned
  unsigned int advanceLevel(int mode = 0) const
  {
    return (mode == 1 ? m_advanceLevel[1] : m_advanceLevel[0]);
  }
  unsigned int localAdvanceLevel(int mode = 0) const
  {
    return (mode == 1 ? m_localAdvanceLevel[1] : m_localAdvanceLevel[0]);
  }
  void setLocalAdvanceLevel(int mode, unsigned int level);
  // Convinence Method that sets both read and write to the same value
  void setLocalAdvanceLevel(unsigned int level);

  // unsetAdvanceLevel causes the item to return its
  // definition advance level information for the specified mode when calling
  // the advanceLevel(mode) method
  void unsetLocalAdvanceLevel(int mode = 0);
  // Returns true if the item is returning its local
  // advance level information
  bool hasLocalAdvanceLevelInfo(int mode = 0) const
  {
    return (mode == 1 ? m_hasLocalAdvanceLevelInfo[1] : m_hasLocalAdvanceLevelInfo[0]);
  }
  const std::string& detailedDescription() const { return m_detailedDescription; }
  void setDetailedDescription(const std::string& text) { m_detailedDescription = text; }

  const std::string& briefDescription() const { return m_briefDescription; }
  void setBriefDescription(const std::string& text) { m_briefDescription = text; }

  virtual smtk::attribute::ItemPtr buildItem(
    Attribute* owningAttribute, int itemPosition) const = 0;
  virtual smtk::attribute::ItemPtr buildItem(
    Item* owningItem, int position, int subGroupPosition) const = 0;
  virtual smtk::attribute::ItemDefinitionPtr createCopy(
    smtk::attribute::ItemDefinition::CopyInfo& info) const = 0;

protected:
  // The constructor must have the value for m_name passed
  // in because that should never change.
  ItemDefinition(const std::string& myname);
  void copyTo(ItemDefinitionPtr def) const;
  virtual void applyCategories(
    const std::set<std::string>& inheritedFromParent, std::set<std::string>& inheritedToParent);
  virtual void applyAdvanceLevels(
    const unsigned int& readLevelFromParent, const unsigned int& writeLevelFromParent);
  int m_version;
  bool m_isOptional;
  bool m_isEnabledByDefault;
  bool m_isOkToInherit;
  std::string m_label;
  std::set<std::string> m_categories;
  std::set<std::string> m_localCategories;
  std::string m_detailedDescription;
  std::string m_briefDescription;
  ItemDefinition::CategoryCheckMode m_categoryCheckMode;
  bool m_hasLocalAdvanceLevelInfo[2];
  unsigned int m_localAdvanceLevel[2];
  unsigned int m_advanceLevel[2];

private:
  // constant value that should never be changed
  const std::string m_name;
};
}
}

#endif /* __smtk_attribute_ItemDefinition_h */
