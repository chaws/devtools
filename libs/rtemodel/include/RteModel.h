#ifndef RteModel_H
#define RteModel_H
/******************************************************************************/
/* RTE - CMSIS Run-Time Environment */
/******************************************************************************/
/** @file RteModel.h
* @brief CMSIS RTE Data Model
*/
/******************************************************************************/
/*
 * Copyright (c) 2020-2021 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/******************************************************************************/
#include "RteItem.h"
#include "RteFile.h"
#include "RteComponent.h"
#include "RteCondition.h"
#include "RteDevice.h"
#include "RtePackage.h"
#include "RteTarget.h"
#include "RteInstance.h"
#include "RteExample.h"
#include "RteBoard.h"
#include "RteGenerator.h"

class RteComponentGroup;
class RteProject;

/**
 * @brief this class represents pack description file *.pdsc or project file *.cprj
*/
class RteModel : public RteItem
{

public:
  /**
   * @brief constructor
   * @param parent pointer to the parent RteItem
   * @param packageState pack state
  */
  RteModel(RteItem* parent, PackageState packageState = PS_INSTALLED);

public:
  /**
   * @brief constructor
   * @param packageState pack state
  */
  RteModel(PackageState packageState = PS_INSTALLED);

  /**
   * @brief virtual destructor
  */
  virtual ~RteModel() override;

  /**
   * @brief cleanup the object
  */
  virtual void ClearModel();

  /**
   * @brief getter for RteCallback object
   * @return RteCallback pointer
  */
  virtual RteCallback* GetCallback() const override;

  /**
   * @brief setter for RteCallback object
   * @param callback RteCallback pointer to set
  */
  virtual void SetCallback(RteCallback* callback);

  /**
   * @brief getter for root directory of installed packages
   * @return root directory
  */
  virtual const std::string& GetRtePath() const { return m_rtePath; }

  /**
   * @brief setter for root directory of installed packages
   * @param rtePath root directory to set
  */
  void SetRtePath(const std::string& rtePath) { m_rtePath = rtePath; }

  /**
   * @brief check if device tree is used by tools displaying and managing packs
   * @return true if device tree is used by tools displaying and managing packs
  */
  bool IsUseDeviceTree() const { return m_bUseDeviceTree; }

  /**
   * @brief setter for usage of device tree by tools displaying and managing packs
   * @param bUse true if device tree is used by tools displaying and managing packs
  */
  void SetUseDeviceTree(bool bUse) { m_bUseDeviceTree = bUse; }

public:
  /**
   * @brief getter for package given by the full package ID
   * @param id full package ID
   * @return RtePackage pointer
  */
  RtePackage* GetPackage(const std::string& id) const;

  /**
   * @brief getter for package with latest version
   * @param id package ID
   * @return RtePackage pointer
  */
  RtePackage* GetLatestPackage(const std::string& id) const;

  /**
   * @brief getter for package with exact the given package ID or the latest if that is younger
   * @param id package ID
   * @return RtePackage pointer
  */
  RtePackage* GetAvailablePackage(const std::string& id) const;

  /**
   * @brief getter for package determined by the given package attributes
   * @param attr package attributes
   * @return RtePackage pointer
  */
  RtePackage* GetPackage(const RteAttributes& attr) const;

  /**
   * @brief get pointer to parent RtePackage
   * @return nullptr since RteModel does not have parent RtePackage
  */
  virtual RtePackage* GetPackage() const override { return nullptr;}

  /**
   * @brief getter for package filter object
   * @return reference to RtePackageFilter object
  */
  const RtePackageFilter& GetPackageFilter() const { return m_packageFilter; }

  /**
   * @brief getter for package filter object
   * @return reference to RtePackageFilter object
  */
  RtePackageFilter& GetPackageFilter() { return m_packageFilter; }

  /**
   * @brief setter for package filter object
   * @param filter reference to RtePackageFilter object to set
  */
  void SetPackageFilter(const RtePackageFilter& filter) { m_packageFilter = filter; }

  /**
   * @brief getter for packages contained in this object
   * @return reference to RtePackageMap object
  */
  const RtePackageMap& GetPackages() const { return m_packages; }

  /**
   * @brief getter for packages with latest version
   * @return reference to RtePackageMap object
  */
  const RtePackageMap& GetLatestPackages() const { return m_latestPackages; }

  /**
   * @brief getter for boards contained in this object
   * @return reference to RteBoardMap object
  */
  const RteBoardMap& GetBoards() const { return m_boards; }

  /**
   * @brief getter for compatible boards given by device
   * @param boards collection of boards to fill
   * @param device given RteDeviceItem pointer
  */
  void GetCompatibleBoards(std::vector<RteBoard*>& boards, RteDeviceItem* device) const;

  /**
   * @brief find board given by the display name
   * @param displayName given display name
   * @return RteBoard pointer
  */
  RteBoard* FindBoard(const std::string& displayName) const;

  /**
   * @brief find compatible board given by display name and device
   * @param displayName given display name
   * @param device given RteDeviceItem object
   * @return RteBoard pointer
  */
  RteBoard* FindCompatibleBoard(const std::string& displayName, RteDeviceItem* device) const;

  /**
   * @brief check if this object has no children
   * @return true if this object has no children
  */
  bool IsEmpty() const { return m_children.size() == 0; }

  /**
   * @brief getter for component by given component ID
   * @param uniqueID component ID
   * @return RteComponent pointer
  */
  RteComponent* GetComponent(const std::string& uniqueID) const;

  /**
   * @brief getter for component by given RteComponentInstance and version to be matched
   * @param ci given RteComponentInstance object
   * @param matchVersion given version to matched
   * @return RteComponent pointer
  */
  RteComponent* GetComponent(RteComponentInstance* ci, bool matchVersion) const;

  /**
   * @brief getter for component. Default implementation returns nullptr
   * @return nullptr in the default implementation
  */
  virtual RteComponent* GetComponent() const override { return nullptr;}

  /**
   * @brief getter for api by given component attributes
   * @param componentAttributes given component attributes
   * @return RteApi pointer
  */
  RteApi* GetApi(const std::map<std::string, std::string>& componentAttributes) const;

  /**
   * @brief getter for api by given api ID
   * @param id api ID
   * @return RteApi pointer
  */
  RteApi* GetApi(const std::string& id) const;

  /**
   * @brief getter for collection of APIs
   * @return collection of api ID mapped to RteApi object pointer
  */
  const std::map<std::string, RteApi* >& GetApiList() const { return m_apiList; }

  /**
   * @brief getter for bundles
   * @return collection of bundle ID mapped RteApi pointer
  */
  const RteBundleMap& GetBundles() const { return m_bundles; }

  /**
   * @brief getter for a bundle by given api ID
   * @param id api ID
   * @return RteBundle pointer
  */
  RteBundle* GetBundle(const std::string& id) const;

  /**
   * @brief getter for bundle with latest version by given bundle name
   * @param name name of bundle
   * @return RteBundle pointer
  */
  RteBundle* GetLatestBundle(const std::string& name) const;

  /**
   * @brief getter for taxonomy object by given taxonomy ID
   * @param id given taxonomy ID
   * @return RteItem pointer
  */
  RteItem* GetTaxonomyItem(const std::string& id) const;

  /**
   * @brief getter for description of a taxonomy object determined by given taxonomy ID
   * @param id given taxonomy ID
   * @return description string of taxonomy
  */
  const std::string& GetTaxonomyDescription(const std::string& id) const;

  /**
   * @brief getter for documentation file of a taxonomy object determined by a given taxonomy ID
   * @param id given taxonomy ID
   * @return file name
  */
  std::string GetTaxonomyDoc(const std::string& id) const;

  /**
   * @brief getter for collection of taxonomy
   * @return collection of taxonomy ID mapped RteItem pointer
  */
  const std::map<std::string, RteItem*>& GetTaxonomy() const { return m_taxonomy; }

  /**
   * @brief getter for number of components
   * @return number of components as integer
  */
  int GetComponentCount() const { return (int)m_componentList.size(); }

  /**
   * @brief getter for collection of components
   * @return collection of component ID mapped to RteComponent pointer
  */
  const RteComponentMap& GetComponentList() const { return m_componentList; }

  /**
   * @brief getter for condition object determined by given package ID and condition ID
   * @param packageId given package ID
   * @param conditionId given condition ID
   * @return RteCondition pointer
  */
  RteCondition* GetCondition(const std::string& packageId, const std::string& conditionId) const;

  /**
   * @brief getter for condition object given by condition ID. Default returns nullptr
   * @param conditionId given condition ID
   * @return nullptr in the default implementation
  */
  virtual RteCondition* GetCondition(const std::string& conditionId) const override { return nullptr;}

  /**
   * @brief getter for condition. Default returns nullptr
   * @return nullptr in the default implementation
  */
  virtual RteCondition* GetCondition() const override { return nullptr;}

  /**
   * @brief getter for collection of device vendors
   * @return collection of vendor ID mapped to RteDeviceVendor pointer
  */
  const std::map<std::string, RteDeviceVendor*>& GetDeviceVendors() const { return m_deviceVendors; }

  /**
   * @brief find vendor by given vendor ID
   * @param vendor given vendor ID
   * @return RteDeviceVendor pointer
  */
  RteDeviceVendor* FindDeviceVendor(const std::string& vendor) const;

  /**
   * @brief check if object representing the given vendor exists, instantiate new one if necessary
   * @param vendor device vendor string
   * @return RteDeviceVendor pointer
  */
  RteDeviceVendor* EnsureDeviceVendor(const std::string& vendor);

  /**
   * @brief add given RteDeviceItem object to device collection
   * @param item given RteDeviceItem pointer
   * @return true if RteDeviceItem object has been added
  */
  bool AddDeviceItem(RteDeviceItem* item);

  /**
   * @brief getter for collection of devices by given device name pattern and vendor name
   * @param devices collection of devices to fill
   * @param namePattern given device name pattern
   * @param vendor given vendor name
   * @param depth tree depth to consider
  */
  void GetDevices(std::list<RteDevice*>& devices, const std::string& namePattern, const std::string& vendor,
                  RteDeviceItem::TYPE depth = RteDeviceItem::DEVICE) const;

  /**
   * @brief getter for device by given device name and vendor name
   * @param deviceName given device name
   * @param vendor given vendor name
   * @return RteDevice pointer
  */
  RteDevice* GetDevice(const std::string& deviceName, const std::string& vendor) const;

  /**
   * @brief getter for number devices
   * @return number of devices as integer
  */
  int GetDeviceCount() const;

  /**
   * @brief getter for number of devices belonging to a given vendor
   * @param vendor given vendor name
   * @return number of devices as integer
  */
  int GetDeviceCount(const std::string& vendor) const;

  /**
   * @brief getter for device tree represented by a RteDeviceItemAggregate object
   * @return RteDeviceItemAggregate pointer
  */
  RteDeviceItemAggregate* GetDeviceTree() const { return m_deviceTree; }

  /**
   * @brief find recursively a device aggregate given by device and vendor name
   * @param name given device name
   * @param vendor given vendor name
   * @return pointer to RteDeviceItemAggregate of type DEVICE, VARIANT or PROCESSOR, nullptr if not found
  */
  RteDeviceItemAggregate* GetDeviceAggregate(const std::string& deviceName, const std::string& vendor) const;

  /**
   *@brief find recursively a device aggregate given by device and vendor
   * @param deviceName given device name
   * @param vendor given vendor name
   * @return pointer to RteDeviceItemAggregate of any type, nullptr if not found
  */
  RteDeviceItemAggregate* GetDeviceItemAggregate(const std::string& name, const std::string& vendor) const;

  /**
   * @brief getter for books given by device and vendor name
   * @param books collection of file path mapped to book title to fill
   * @param device given device name
   * @param vendor given vendor name
  */
  void GetBoardBooks(std::map<std::string, std::string>& books, const std::string& device, const std::string& vendor) const;

  /**
   * @brief getter for books given by device attributes
   * @param books collection of file path mapped to book title to fill
   * @param deviceAttributes device attributes
  */
  void GetBoardBooks(std::map<std::string, std::string>& books, const std::map<std::string, std::string>& deviceAttributes) const;

public:
  /**
   * @brief getter for this pointer
   * @return this pointer
  */
  virtual RteModel* GetModel() const override{ return const_cast<RteModel*>(this); }

  /**
   * @brief getter for package state
   * @return enumerator of type PackageState
  */
  virtual PackageState GetPackageState() const override{ return m_packageState; }

  /**
   * @brief clean up this object
  */
  virtual void Clear() override;

  /**
   * @brief construct CMSIS RTE data model given by XMLTreeElement object
   * @param xmlTree given XMLTreeElement object
   * @return true if successful
  */
  virtual bool Construct(XMLTreeElement* xmlTree) override;

  /**
   * @brief validate this object
   * @return true if validation is successful
  */
  virtual bool Validate() override;

public:
  /**
   * @brief add given component to this instance
   * @param c given component pointer to add
  */
  void InsertComponent(RteComponent* c);

  /**
   * @brief add given bundle to this instance
   * @param b given bundle pointer to add
  */
  void InsertBundle(RteBundle* b);

  /**
   * @brief getter for context for condition evaluation
   * @return RteConditionContext pointer
  */
  RteConditionContext* GetFilterContext() const { return m_filterContext; }

  /**
   * @brief setter for context for condition evaluation
   * @param filterContext RteConditionContext pointer to set
  */
  void SetFilterContext(RteConditionContext* filterContext) { m_filterContext = filterContext; }

  /**
   * @brief check if supplied item passes current filter context
   * @param item RteItem to check
   * @return true if passes or no filtering is active
  */
  bool IsFiltered(RteItem* item);

  /**
   * @brief filter this object with given RteModel
   * @param globalModel given RteModel
   * @param devicePackage given device package to be considered during filtering
   * @return pointer to effective device package of type RtePackage
  */
  RtePackage* FilterModel(RteModel* globalModel, RtePackage* devicePackage);

  /**
   * @brief create pack object depending on the given tag
   * @param tag given tag
   * @return pointer to created pack of type RtePackage
  */
  virtual RtePackage* CreatePackage(const std::string& tag);

  /**
   * @brief construct packs out of the given XMLTreeElement object.
   * Created packs are stored in the given collection
   * @param xmlTree given XMLTreeElement object
   * @param packs collection of RtePackage pointer to fill
   * @return true if construction successful
  */
  virtual bool ConstructPacks(XMLTreeElement* xmlTree, std::list<RtePackage*>& packs);

  /**
   * @brief construct pack out of the given XMLTreeElement object
   * @param xmlTreeDoc given XMLTreeElement object
   * @return pointer to created pack of type RtePackage
  */
  virtual RtePackage* ConstructPack(XMLTreeElement* xmlTreeDoc);

  /**
   * @brief insert given collection of packs into internal collection
   * @param packs given collection of packs
  */
  void InsertPacks(const std::list<RtePackage*>& packs);

  /**
   * @brief insert a given pack into internal collection
   * @param package given pack
  */
  void InsertPack(RtePackage* package);

protected:

  void ClearDevices();

  virtual void FillComponentList(RtePackage* devicePackage);
  virtual void AddItemsFromPack(RtePackage* pack); // adds taxonomy and components
  virtual void FillDeviceTree();
  virtual void FillDeviceTree(RtePackage* pack);

  bool IsApiDominatingOrNewer(RteApi* a);

protected:
  PackageState m_packageState;

protected:
  std::map<std::string, RteApi* > m_apiList; // flat std::list of available APIs
  RteComponentMap m_componentList; // full unique component std::list
  std::map<std::string, RteItem*> m_taxonomy; // flat std::list of standard Class descriptions
  RteBundleMap m_bundles;

  std::map<std::string, RteDeviceVendor*> m_deviceVendors;

  RteDeviceItemAggregate* m_deviceTree;// vendor/family/subfamily/device/variant/processor
  bool m_bUseDeviceTree; // flag is set to true by Pack Installer, uVision does not use RteDeviceItemAggregate items any more
  RteBoardMap m_boards;

  RteCallback* m_callback;

  RtePackageMap m_packages; // sorted package std::list (full id to package, latest versions first)
  RtePackageMap m_latestPackages; // latests packages (common id to package)

  std::list<RtePackage*> m_packageDuplicates;

  RtePackageFilter m_packageFilter;
  RteConditionContext* m_filterContext; // constructed, updated and deleted by target

  std::string m_rtePath; // path to RTEPATH from tools.ini
};

/**
 * @brief this class manages all loaded instances associated with pack description file *.pdsc and project file *.cprj
*/
class RteGlobalModel : public RteModel
{
public:
  /**
   * @brief constructor
  */
  RteGlobalModel();

  /**
   * @brief destructor
  */
  virtual ~RteGlobalModel();

  /**
   * @brief clean up CMSIS RTE data model and loaded projects
  */
  virtual void Clear() override;

  /**
   * @brief clean up all project targets and CMSIS RTE data model
  */
  virtual void ClearModel() override;

  /**
   * @brief setter for RteCallback object
   * @param callback given RteCallback object to set
  */
  virtual void SetCallback(RteCallback* callback) override;

public:
  /**
   * @brief getter for collection of loaded projects
   * @return collection of project ID mapped to RteProject pointer
  */
  const std::map<int, RteProject*>& GetProjects() const { return m_projects; }

  /**
   * @brief getter for project given by it's ID
   * @param id given project ID
   * @return RteProject pointer
  */
  RteProject* GetProject(int id) const;

  /**
   * @brief add a new project to this instance
   * @param id project ID
   * @param project pointer to RteProject or nullptr to create a new one
   * @return RteProject pointer
  */
  RteProject* AddProject(int id, RteProject* project = nullptr);

  /**
   * @brief delete a project given by it's ID
   * @param id given project ID
  */
  void DeleteProject(int id);

  /**
   * @brief getter for a project. Default returns nullptr
   * @return
  */
  virtual RteProject* GetProject() const override { return nullptr;}

  /**
   * @brief remove all projects
  */
  void ClearProjects();

  /**
   * @brief clear all targets of the project given by it's ID
   * @param id given project ID or -1 to remove targets of all projects
  */
  void ClearProjectTargets(int id = -1);

  /**
   * @brief getter for active project given by it's ID
   * @return RteProject pointer
  */
  RteProject* GetActiveProject() const { return GetProject(m_nActiveProjectId); }

  /**
   * @brief getter for ID of the active project
   * @return project ID as integer
  */
  int GetActiveProjectId() const { return m_nActiveProjectId; }

  /**
   * @brief setter for ID of active project
   * @param id given project ID to set
  */
  void SetActiveProjectId(int id) { m_nActiveProjectId = id; }


protected:
  int GenerateProjectId();

protected:

  std::map<int, RteProject*> m_projects;
  int m_nActiveProjectId; // 1-based project id
};


class RteGeneratorModel : public RteModel
{
public:
  RteGeneratorModel(RteItem* parent);

public:
  RteGeneratorModel();
  virtual ~RteGeneratorModel() override;

  virtual void ClearModel() override;


public:
  RtePackage* GetGpdscPack() const { return m_gpdscPack; }
  RteGenerator* GetGenerator() const { return m_generator; }

  virtual void Clear() override;
  virtual const std::string& GetGeneratorName() const override;

protected:
  virtual void AddItemsFromPack(RtePackage* pack) override; // adds taxonomy and components

protected:
  RtePackage* m_gpdscPack; // only one gpdsc/cprj is allowed for generated model
  RteGenerator* m_generator; // only one generator is allowed for generated model
};

#endif // RteModel_H
