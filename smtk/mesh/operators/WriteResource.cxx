//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#include "smtk/mesh/operators/WriteResource.h"

#include "smtk/attribute/Attribute.h"
#include "smtk/attribute/FileItem.h"
#include "smtk/attribute/IntItem.h"
#include "smtk/attribute/ResourceItem.h"
#include "smtk/attribute/VoidItem.h"

#include "smtk/common/Archive.h"
#include "smtk/common/CompilerInformation.h"
#include "smtk/common/Paths.h"

#include "smtk/mesh/WriteResource_xml.h"
#include "smtk/mesh/core/Resource.h"

#include "smtk/mesh/operators/Write.h"

#include "smtk/mesh/json/jsonResource.h"

SMTK_THIRDPARTY_PRE_INCLUDE
#include "nlohmann/json.hpp"

//force to use filesystem version 3
#define BOOST_FILESYSTEM_VERSION 3
#include <boost/filesystem.hpp>
SMTK_THIRDPARTY_POST_INCLUDE

#include <fstream>

using namespace smtk::model;

namespace
{
void cleanup(const std::string& file_path)
{
  //first verify the file exists
  ::boost::filesystem::path path(file_path);
  if (::boost::filesystem::is_regular_file(path))
  {
    //remove the file_path if it exists.
    ::boost::filesystem::remove(path);
  }
}
}

namespace smtk
{
namespace mesh
{

bool WriteResource::ableToOperate()
{
  if (!this->smtk::operation::XMLOperation::ableToOperate())
  {
    return false;
  }

  if (this->parameters()->associations()->numberOfValues() < 1)
  {
    return false;
  }

  return true;
}

WriteResource::Result WriteResource::operateInternal()
{
  auto resourceItem = this->parameters()->associations();

  smtk::mesh::Resource::Ptr resource =
    std::dynamic_pointer_cast<smtk::mesh::Resource>(resourceItem->value());

  // Serialize resource into a set of JSON records:
  nlohmann::json j = resource;
  if (j.is_null())
  {
    smtkErrorMacro(log(), "Unable to serialize mesh to json object.");
    return this->createResult(smtk::operation::Operation::Outcome::FAILED);
  }

  // Check if the mesh files are to be archived into a single file
  auto archive = this->parameters()->findVoid("archive");
  if (archive && archive->isEnabled())
  {
    std::string smtkFilename = "index.json";
    std::string meshFilename = "mesh.h5m";
    std::string tmpsmtkPath = smtk::common::Paths::tempDirectory() + "/" + smtkFilename;
    std::string tmpMeshPath = smtk::common::Paths::tempDirectory() + "/" + meshFilename;
    j["Mesh URL"] = meshFilename;

    // Write the smtk index
    {
      std::ofstream file(tmpsmtkPath);
      if (!file.good())
      {
        smtkErrorMacro(log(), "Unable to open \"" << tmpsmtkPath << "\" for writing.");
        return this->createResult(smtk::operation::Operation::Outcome::FAILED);
      }
      file << j.dump(2);
      file.close();
    }

    // Write the mesh file
    {
      // Create a write operator
      smtk::mesh::Write::Ptr writeOp = smtk::mesh::Write::create();
      writeOp->parameters()->findFile("filename")->setValue(tmpMeshPath);

      // Set the entity association
      writeOp->parameters()->associate(resource);

      // Execute the operation
      smtk::operation::Operation::Result writeOpResult = writeOp->operate(Key());

      // Test for success
      if (writeOpResult->findInt("outcome")->value() !=
        static_cast<int>(smtk::operation::Operation::Outcome::SUCCEEDED))
      {
        smtkErrorMacro(log(), "Unable to write mesh to  \"" << tmpMeshPath << "\".");
        cleanup(tmpsmtkPath);
        return this->createResult(smtk::operation::Operation::Outcome::FAILED);
      }
    }

    // Populate an archive with the smtk index and mesh file
    {
      smtk::common::Archive archive(resource->location());
      archive.insert(tmpsmtkPath, smtkFilename);
      archive.insert(tmpMeshPath, meshFilename);
      if (!archive.archive())
      {
        smtkErrorMacro(log(), "Unable to archive files to \"" + resource->location() + "\".");
        cleanup(tmpsmtkPath);
        cleanup(tmpMeshPath);
        return this->createResult(smtk::operation::Operation::Outcome::FAILED);
      }
    }

    // Clean up temporary files.
    {
      cleanup(tmpsmtkPath);
      cleanup(tmpMeshPath);
    }

    return this->createResult(smtk::operation::Operation::Outcome::SUCCEEDED);
  }
  else
  {
    std::string meshFilename = smtk::common::Paths::stem(resource->location()) + ".h5m";
    std::string meshPath = smtk::common::Paths::replaceExtension(resource->location(), ".h5m");

    j["Mesh URL"] = meshFilename;

    {
      std::ofstream file(resource->location());
      if (!file.good())
      {
        smtkErrorMacro(log(), "Unable to open \"" << resource->location() << "\" for writing.");
        return this->createResult(smtk::operation::Operation::Outcome::FAILED);
      }
      file << j.dump(2);
      file.close();
    }

    // Create a write operator
    smtk::mesh::Write::Ptr writeOp = smtk::mesh::Write::create();
    writeOp->parameters()->findFile("filename")->setValue(meshPath);

    // Set the entity association
    writeOp->parameters()->associate(resource);

    // Execute the operation
    smtk::operation::Operation::Result writeOpResult = writeOp->operate(Key());

    // Test for success
    if (writeOpResult->findInt("outcome")->value() !=
      static_cast<int>(smtk::operation::Operation::Outcome::SUCCEEDED))
    {
      smtkErrorMacro(log(), "Unable to write files to \"" + meshPath + "\".");
      return this->createResult(smtk::operation::Operation::Outcome::FAILED);
    }

    // Add the mesh file to the result's list of additional files
    auto result = this->createResult(smtk::operation::Operation::Outcome::SUCCEEDED);
    result->findFile("additional files")->appendValue(meshPath);

    return result;
  }
}

const char* WriteResource::xmlDescription() const
{
  return WriteResource_xml;
}

void WriteResource::markModifiedResources(WriteResource::Result& /*unused*/)
{
  auto resourceItem = this->parameters()->associations();
  for (auto rit = resourceItem->begin(); rit != resourceItem->end(); ++rit)
  {
    auto resource = std::dynamic_pointer_cast<smtk::resource::Resource>(*rit);

    // Set the resource as unmodified from its persistent (i.e. on-disk) state
    resource->setClean(true);
  }
}

bool write(const smtk::resource::ResourcePtr& resource)
{
  WriteResource::Ptr write = WriteResource::create();
  write->parameters()->associate(resource);
  WriteResource::Result result = write->operate();
  return (
    result->findInt("outcome")->value() == static_cast<int>(WriteResource::Outcome::SUCCEEDED));
}
}
}
