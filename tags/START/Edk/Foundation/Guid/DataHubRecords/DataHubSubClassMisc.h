/*++
 
Copyright 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DataHubSubClassMisc.h
    
Abstract:

  Definitions for Misc sub class data records

Revision History

--*/

#ifndef _DATAHUB_SUBCLASS_MISC_H_
#define _DATAHUB_SUBCLASS_MISC_H_

#include EFI_GUID_DEFINITION(DataHubRecords)

#define EFI_MISC_SUBCLASS_GUID \
{ 0x772484B2, 0x7482, 0x4b91, 0x9F, 0x9A, 0xAD, 0x43, 0xF8, 0x1C, 0x58, 0x81 }

#define EFI_MISC_SUBCLASS_VERSION     0x0100

#pragma pack(1)

//////////////////////////////////////////////////////////////////////////////
//
// Last PCI Bus Number
//
#define EFI_MISC_LAST_PCI_BUS_RECORD_NUMBER   0x00000001

typedef struct {
  UINT8   LastPciBus;
} EFI_MISC_LAST_PCI_BUS;


//////////////////////////////////////////////////////////////////////////////
//
// Misc. BIOS Vendor - SMBIOS Type 0
//
#define EFI_MISC_BIOS_VENDOR_RECORD_NUMBER  0x00000002

typedef struct {
  UINT32  Reserved1                         :2;
  UINT32  Unknown                           :1;
  UINT32  BiosCharacteristicsNotSupported   :1;
  UINT32  IsaIsSupported                    :1;
  UINT32  McaIsSupported                    :1;
  UINT32  EisaIsSupported                   :1;
  UINT32  PciIsSupported                    :1;
  UINT32  PcmciaIsSupported                 :1;
  UINT32  PlugAndPlayIsSupported            :1;
  UINT32  ApmIsSupported                    :1;
  UINT32  BiosIsUpgradable                  :1;
  UINT32  BiosShadowingAllowed              :1;
  UINT32  VlVesaIsSupported                 :1;
  UINT32  EscdSupportIsAvailable            :1;
  UINT32  BootFromCdIsSupported             :1;
  UINT32  SelectableBootIsSupported         :1;
  UINT32  RomBiosIsSocketed                 :1;
  UINT32  BootFromPcmciaIsSupported         :1;
  UINT32  EDDSpecificationIsSupported       :1;
  UINT32  JapaneseNecFloppyIsSupported      :1;
  UINT32  JapaneseToshibaFloppyIsSupported  :1;
  UINT32  Floppy525_360IsSupported          :1;
  UINT32  Floppy525_12IsSupported           :1;
  UINT32  Floppy35_720IsSupported           :1;
  UINT32  Floppy35_288IsSupported           :1;
  UINT32  PrintScreenIsSupported            :1;
  UINT32  Keyboard8042IsSupported           :1;
  UINT32  SerialIsSupported                 :1;
  UINT32  PrinterIsSupported                :1;
  UINT32  CgaMonoIsSupported                :1;
  UINT32  NecPc98                           :1;
  UINT32  AcpiIsSupported                   :1;
  UINT32  UsbLegacyIsSupported              :1;
  UINT32  AgpIsSupported                    :1;
  UINT32  I20BootIsSupported                :1;
  UINT32  Ls120BootIsSupported              :1;
  UINT32  AtapiZipDriveBootIsSupported      :1;
  UINT32  Boot1394IsSupported               :1;
  UINT32  SmartBatteryIsSupported           :1;
  UINT32  BiosBootSpecIsSupported           :1;
  UINT32  FunctionKeyNetworkBootIsSupported :1;
  UINT32  Reserved                          :22;
} EFI_MISC_BIOS_CHARACTERISTICS;

typedef struct {
  UINT32  BiosReserved                      :16;
  UINT32  SystemReserved                    :16;
  UINT32  Reserved                          :32;
} EFI_MISC_BIOS_CHARACTERISTICS_EXTENSION;

typedef struct {
  STRING_REF                      BiosVendor;
  STRING_REF                      BiosVersion;
  STRING_REF                      BiosReleaseDate;
  EFI_PHYSICAL_ADDRESS            BiosStartingAddress;
  EFI_EXP_BASE2_DATA              BiosPhysicalDeviceSize;
  EFI_MISC_BIOS_CHARACTERISTICS   BiosCharacteristics1;
  EFI_MISC_BIOS_CHARACTERISTICS_EXTENSION  BiosCharacteristics2;
} EFI_MISC_BIOS_VENDOR;       


//////////////////////////////////////////////////////////////////////////////
//
// Misc. System Manufacturer - SMBIOS Type 1
//
#define EFI_MISC_SYSTEM_MANUFACTURER_RECORD_NUMBER 0x00000003

typedef enum {  
  EfiSystemWakeupTypeReserved = 0,
  EfiSystemWakeupTypeOther = 1,
  EfiSystemWakeupTypeUnknown = 2,
  EfiSystemWakeupTypeApmTimer = 3,
  EfiSystemWakeupTypeModemRing = 4,
  EfiSystemWakeupTypeLanRemote = 5,
  EfiSystemWakeupTypePowerSwitch = 6,
  EfiSystemWakeupTypePciPme = 7,
  EfiSystemWakeupTypeAcPowerRestored = 8,
} EFI_MISC_SYSTEM_WAKEUP_TYPE;

typedef struct {
  STRING_REF                      SystemManufacturer;
  STRING_REF                      SystemProductName;
  STRING_REF                      SystemVersion;
  STRING_REF                      SystemSerialNumber;
  EFI_GUID                        SystemUuid;
  EFI_MISC_SYSTEM_WAKEUP_TYPE     SystemWakeupType;
} EFI_MISC_SYSTEM_MANUFACTURER;       


//////////////////////////////////////////////////////////////////////////////
//
// Misc. Base Board Manufacturer - SMBIOS Type 2
//
#define EFI_MISC_BASE_BOARD_MANUFACTURER_RECORD_NUMBER 0x00000004

typedef struct {
  UINT32  Motherboard           :1;
  UINT32  RequiresDaughterCard  :1;
  UINT32  Removable             :1;
  UINT32  Replaceable           :1;
  UINT32  HotSwappable          :1;
  UINT32  Reserved              :27;
} EFI_BASE_BOARD_FEATURE_FLAGS;

typedef enum {  
  EfiBaseBoardTypeUnknown = 1,
  EfiBaseBoardTypeOther = 2,
  EfiBaseBoardTypeServerBlade = 3,
  EfiBaseBoardTypeConnectivitySwitch = 4,
  EfiBaseBoardTypeSystemManagementModule = 5,
  EfiBaseBoardTypeProcessorModule = 6,
  EfiBaseBoardTypeIOModule = 7,
  EfiBaseBoardTypeMemoryModule = 8,
  EfiBaseBoardTypeDaughterBoard = 9,
  EfiBaseBoardTypeMotherBoard = 0xA,
  EfiBaseBoardTypeProcessorMemoryModule = 0xB,
  EfiBaseBoardTypeProcessorIOModule = 0xC,
  EfiBaseBoardTypeInterconnectBoard = 0xD,
} EFI_BASE_BOARD_TYPE;

typedef struct {
  STRING_REF                      BaseBoardManufacturer;
  STRING_REF                      BaseBoardProductName;
  STRING_REF                      BaseBoardVersion;
  STRING_REF                      BaseBoardSerialNumber;
  STRING_REF                      BaseBoardAssetTag;
  STRING_REF                      BaseBoardChassisLocation;
  EFI_BASE_BOARD_FEATURE_FLAGS    BaseBoardFeatureFlags;
  EFI_BASE_BOARD_TYPE             BaseBoardType;
  EFI_INTER_LINK_DATA             BaseBoardChassisLink;
  UINT32                          BaseBoardNumberLinks;
  EFI_INTER_LINK_DATA             LinkN;
} EFI_MISC_BASE_BOARD_MANUFACTURER;       


//////////////////////////////////////////////////////////////////////////////
//
// Misc. System/Chassis Enclosure - SMBIOS Type 3
//
#define EFI_MISC_CHASSIS_MANUFACTURER_RECORD_NUMBER  0x00000005

typedef enum {  
  EfiMiscChassisTypeOther = 0x1,
  EfiMiscChassisTypeUnknown = 0x2,
  EfiMiscChassisTypeDeskTop = 0x3,
  EfiMiscChassisTypeLowProfileDesktop = 0x4,
  EfiMiscChassisTypePizzaBox = 0x5,
  EfiMiscChassisTypeMiniTower = 0x6,
  EfiMiscChassisTypeTower = 0x7,
  EfiMiscChassisTypePortable = 0x8,
  EfiMiscChassisTypeLapTop = 0x9,
  EfiMiscChassisTypeNotebook = 0xA,
  EfiMiscChassisTypeHandHeld = 0xB,
  EfiMiscChassisTypeDockingStation = 0xC,
  EfiMiscChassisTypeAllInOne = 0xD,
  EfiMiscChassisTypeSubNotebook = 0xE,
  EfiMiscChassisTypeSpaceSaving = 0xF,
  EfiMiscChassisTypeLunchBox = 0x10,
  EfiMiscChassisTypeMainServerChassis = 0x11,
  EfiMiscChassisTypeExpansionChassis = 0x12,
  EfiMiscChassisTypeSubChassis = 0x13,
  EfiMiscChassisTypeBusExpansionChassis = 0x14,
  EfiMiscChassisTypePeripheralChassis = 0x15,
  EfiMiscChassisTypeRaidChassis = 0x16,
  EfiMiscChassisTypeRackMountChassis = 0x17,
  EfiMiscChassisTypeSealedCasePc = 0x18,
  EfiMiscChassisMultiSystemChassis = 0x19,
} EFI_MISC_CHASSIS_TYPE;

typedef struct {
  UINT32  ChassisType       :16;
  UINT32  ChassisLockPresent:1;
  UINT32  Reserved          :15;
} EFI_MISC_CHASSIS_STATUS;

typedef enum {  
  EfiChassisStateOther = 1,
  EfiChassisStateUnknown = 2,
  EfiChassisStateSafe = 3,
  EfiChassisStateWarning = 4,
  EfiChassisStateCritical = 5,
  EfiChassisStateNonRecoverable = 6,
} EFI_MISC_CHASSIS_STATE;

typedef enum {  
  EfiChassisSecurityStatusOther = 1,
  EfiChassisSecurityStatusUnknown = 2,
  EfiChassisSecurityStatusNone = 3,
  EfiChassisSecurityStatusExternalInterfaceLockedOut = 4,
  EfiChassisSecurityStatusExternalInterfaceLockedEnabled = 5,
} EFI_MISC_CHASSIS_SECURITY_STATE;

typedef struct {
  UINT32  RecordType  :1;
  UINT32  Reserved    :24;
} EFI_MISC_ELEMENT_TYPE;

typedef struct {
  EFI_MISC_ELEMENT_TYPE   ChassisElementType;
  EFI_INTER_LINK_DATA     ChassisElementStructure;
  EFI_BASE_BOARD_TYPE     ChassisBaseBoard;
  UINT32                  ChassisElementMinimum;
  UINT32                  ChassisElementMaximum;
} EFI_MISC_ELEMENTS; 

typedef struct {
  STRING_REF                      ChassisManufacturer;
  STRING_REF                      ChassisVersion;
  STRING_REF                      ChassisSerialNumber;
  STRING_REF                      ChassisAssetTag;
  EFI_MISC_CHASSIS_STATUS         ChassisType;
  EFI_MISC_CHASSIS_STATE          ChassisBootupState;
  EFI_MISC_CHASSIS_STATE          ChassisPowerSupplyState;
  EFI_MISC_CHASSIS_STATE          ChassisThermalState;
  EFI_MISC_CHASSIS_SECURITY_STATE ChassisSecurityState;
  UINT32                          ChassisOemDefined;
  UINT32                          ChassisHeight;
  UINT32                          ChassisNumberPowerCords;
  UINT32                          ChassisElementCount;
  UINT32                          ChassisElementRecordLength;//
  EFI_MISC_ELEMENTS               ChassisElements;
} EFI_MISC_CHASSIS_MANUFACTURER;


//////////////////////////////////////////////////////////////////////////////
//
// Misc. Port Connector Information - SMBIOS Type 8
//
#define EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_RECORD_NUMBER 0x00000006

typedef enum {  
  EfiPortConnectorTypeNone = 0x0,
  EfiPortConnectorTypeCentronics = 0x1,
  EfiPortConnectorTypeMiniCentronics = 0x2,
  EfiPortConnectorTypeProprietary = 0x3,
  EfiPortConnectorTypeDB25Male = 0x4,
  EfiPortConnectorTypeDB25Female = 0x5,
  EfiPortConnectorTypeDB15Male = 0x6,
  EfiPortConnectorTypeDB15Female = 0x7,
  EfiPortConnectorTypeDB9Male = 0x8,
  EfiPortConnectorTypeDB9Female = 0x9,
  EfiPortConnectorTypeRJ11 = 0xA,
  EfiPortConnectorTypeRJ45 = 0xB,
  EfiPortConnectorType50PinMiniScsi = 0xC,
  EfiPortConnectorTypeMiniDin = 0xD,
  EfiPortConnectorTypeMicriDin = 0xE,
  EfiPortConnectorTypePS2 = 0xF,
  EfiPortConnectorTypeInfrared = 0x10,
  EfiPortConnectorTypeHpHil = 0x11,
  EfiPortConnectorTypeUsb = 0x12,
  EfiPortConnectorTypeSsaScsi = 0x13,
  EfiPortConnectorTypeCircularDin8Male = 0x14,
  EfiPortConnectorTypeCircularDin8Female = 0x15,
  EfiPortConnectorTypeOnboardIde = 0x16,
  EfiPortConnectorTypeOnboardFloppy = 0x17,
  EfiPortConnectorType9PinDualInline = 0x18,
  EfiPortConnectorType25PinDualInline = 0x19,
  EfiPortConnectorType50PinDualInline = 0x1A,
  EfiPortConnectorType68PinDualInline = 0x1B,
  EfiPortConnectorTypeOnboardSoundInput = 0x1C,
  EfiPortConnectorTypeMiniCentronicsType14 = 0x1D,
  EfiPortConnectorTypeMiniCentronicsType26 = 0x1E,
  EfiPortConnectorTypeHeadPhoneMiniJack = 0x1F,
  EfiPortConnectorTypeBNC = 0x20,
  EfiPortConnectorType1394 = 0x21,
  EfiPortConnectorTypePC98 = 0xA0,
  EfiPortConnectorTypePC98Hireso = 0xA1,
  EfiPortConnectorTypePCH98 = 0xA2,
  EfiPortConnectorTypePC98Note = 0xA3,
  EfiPortConnectorTypePC98Full = 0xA4,
  EfiPortConnectorTypeOther = 0xFF,
} EFI_MISC_PORT_CONNECTOR_TYPE;

typedef enum {  
  EfiPortTypeNone = 0x0,
  EfiPortTypeParallelXtAtCompatible = 0x1,
  EfiPortTypeParallelPortPs2 = 0x2,
  EfiPortTypeParallelPortEcp = 0x3,
  EfiPortTypeParallelPortEpp = 0x4,
  EfiPortTypeParallelPortEcpEpp = 0x5,
  EfiPortTypeSerialXtAtCompatible = 0x6,
  EfiPortTypeSerial16450Compatible = 0x7,
  EfiPortTypeSerial16550Compatible = 0x8,
  EfiPortTypeSerial16550ACompatible = 0x9,
  EfiPortTypeScsi = 0xA,
  EfiPortTypeMidi = 0xB,
  EfiPortTypeJoyStick = 0xC,
  EfiPortTypeKeyboard = 0xD,
  EfiPortTypeMouse = 0xE,
  EfiPortTypeSsaScsi = 0xF,
  EfiPortTypeUsb = 0x10,
  EfiPortTypeFireWire = 0x11,
  EfiPortTypePcmciaTypeI = 0x12,
  EfiPortTypePcmciaTypeII = 0x13,
  EfiPortTypePcmciaTypeIII = 0x14,
  EfiPortTypeCardBus = 0x15,
  EfiPortTypeAccessBusPort = 0x16,
  EfiPortTypeScsiII = 0x17,
  EfiPortTypeScsiWide = 0x18,
  EfiPortTypePC98 = 0x19,
  EfiPortTypePC98Hireso = 0x1A,
  EfiPortTypePCH98 = 0x1B,
  EfiPortTypeVideoPort = 0x1C,
  EfiPortTypeAudioPort = 0x1D,
  EfiPortTypeModemPort = 0x1E,
  EfiPortTypeNetworkPort = 0x1F,
  EfiPortType8251Compatible = 0xA0,
  EfiPortType8251FifoCompatible = 0xA1,
  EfiPortTypeOther = 0xFF,
} EFI_MISC_PORT_TYPE;


typedef struct {
  EFI_STRING_TOKEN              PortInternalConnectorDesignator;
  EFI_STRING_TOKEN              PortExternalConnectorDesignator;
  EFI_MISC_PORT_CONNECTOR_TYPE  PortInternalConnectorType;
  EFI_MISC_PORT_CONNECTOR_TYPE  PortExternalConnectorType;
  EFI_MISC_PORT_TYPE            PortType;
  //EFI_DEVICE_PATH_PROTOCOL      PortPath;
  EFI_MISC_PORT_DEVICE_PATH      PortPath;
} EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR;      


//////////////////////////////////////////////////////////////////////////////
//
// Misc. System Slots - SMBIOS Type 9
//
#define EFI_MISC_SYSTEM_SLOT_DESIGNATION_RECORD_NUMBER 0x00000007

typedef enum {  
  EfiSlotTypeOther = 0x1,
  EfiSlotTypeUnknown = 0x2,
  EfiSlotTypeIsa = 0x3,
  EfiSlotTypeMca = 0x4,
  EfiSlotTypeEisa = 0x5,
  EfiSlotTypePci = 0x6,
  EfiSlotTypePcmcia = 0x7,
  EfiSlotTypeVlVesa = 0x8,
  EfiSlotTypeProprietary = 0x9,
  EfiSlotTypeProcessorCardSlot = 0xA,
  EfiSlotTypeProprietaryMemoryCardSlot = 0xB,
  EfiSlotTypeIORiserCardSlot = 0xC,
  EfiSlotTypeNuBus = 0xD,
  EfiSlotTypePci66MhzCapable = 0xE,
  EfiSlotTypeAgp = 0xF,
  EfiSlotTypeApg2X = 0x10,
  EfiSlotTypeAgp4X = 0x11,
  EfiSlotTypePciX = 0x12,
  EfiSlotTypeAgp4x = 0x13,
  EfiSlotTypePC98C20 = 0xA0,
  EfiSlotTypePC98C24 = 0xA1,
  EfiSlotTypePC98E = 0xA2,
  EfiSlotTypePC98LocalBus = 0xA3,
  EfiSlotTypePC98Card = 0xA4,
} EFI_MISC_SLOT_TYPE;

typedef enum {  
  EfiSlotDataBusWidthOther = 1,
  EfiSlotDataBusWidthUnknown = 2,
  EfiSlotDataBusWidth8Bit = 3,
  EfiSlotDataBusWidth16Bit = 4,
  EfiSlotDataBusWidth32Bit = 5,
  EfiSlotDataBusWidth64Bit = 6,
  EfiSlotDataBusWidth128Bit = 7,
} EFI_MISC_SLOT_DATA_BUS_WIDTH;

typedef enum {  
  EfiSlotUsageOther = 1,
  EfiSlotUsageUnknown = 2,
  EfiSlotUsageAvailable = 3,
  EfiSlotUsageInUse = 4,
} EFI_MISC_SLOT_USAGE;
  
typedef enum {  
  EfiSlotLengthOther = 1,
  EfiSlotLengthUnknown = 2,
  EfiSlotLengthShort = 3,
  EfiSlotLengthLong = 4
} EFI_MISC_SLOT_LENGTH;

typedef struct {
  UINT32  CharacteristicsUnknown  :1;
  UINT32  Provides50Volts         :1;
  UINT32  Provides33Volts         :1;
  UINT32  SharedSlot              :1;
  UINT32  PcCard16Supported       :1;
  UINT32  CardBusSupported        :1;
  UINT32  ZoomVideoSupported      :1;
  UINT32  ModemRingResumeSupported:1;
  UINT32  PmeSignalSupported      :1;
  UINT32  HotPlugDevicesSupported :1;
  UINT32  SmbusSignalSupported    :1;
  UINT32  Reserved                :21;
} EFI_MISC_SLOT_CHARACTERISTICS;

typedef struct {
  STRING_REF                    SlotDesignation;
  EFI_MISC_SLOT_TYPE            SlotType;
  EFI_MISC_SLOT_DATA_BUS_WIDTH  SlotDataBusWidth;
  EFI_MISC_SLOT_USAGE           SlotUsage;
  EFI_MISC_SLOT_LENGTH          SlotLength;
  UINT16                        SlotId;
  EFI_MISC_SLOT_CHARACTERISTICS SlotCharacteristics;
  EFI_DEVICE_PATH_PROTOCOL      SlotDevicePath;
} EFI_MISC_SYSTEM_SLOT_DESIGNATION;      


//////////////////////////////////////////////////////////////////////////////
//
// Misc. Onboard Device - SMBIOS Type 10
//
#define EFI_MISC_ONBOARD_DEVICE_RECORD_NUMBER 0x00000008

typedef enum {  
  EfiOnBoardDeviceTypeOther = 1,
  EfiOnBoardDeviceTypeUnknown = 2,
  EfiOnBoardDeviceTypeVideo = 3,
  EfiOnBoardDeviceTypeScsiController = 4,
  EfiOnBoardDeviceTypeEthernet = 5,
  EfiOnBoardDeviceTypeTokenRing = 6,
  EfiOnBoardDeviceTypeSound = 7,
} EFI_MISC_ONBOARD_DEVICE_TYPE;

typedef struct {
  UINT32  DeviceType    :16;
  UINT32  DeviceEnabled :1;
  UINT32  Reserved      :15;
} EFI_MISC_ONBOARD_DEVICE_STATUS;

typedef struct {
  STRING_REF                      OnBoardDeviceDescription;
  EFI_MISC_ONBOARD_DEVICE_STATUS  OnBoardDeviceStatus;
  EFI_DEVICE_PATH_PROTOCOL        OnBoardDevicePath;
} EFI_MISC_ONBOARD_DEVICE;


//////////////////////////////////////////////////////////////////////////////
//
// Misc. BIOS Language Information - SMBIOS Type 11
//
#define EFI_MISC_OEM_STRING_RECORD_NUMBER 0x00000009

typedef struct {
  STRING_REF                          OemStringRef[1];
} EFI_MISC_OEM_STRING;


//////////////////////////////////////////////////////////////////////////////
//
// Misc. System Options - SMBIOS Type 12
//
typedef struct {
  STRING_REF                          SystemOptionStringRef[1];
} EFI_MISC_SYSTEM_OPTION_STRING;      

#define EFI_MISC_SYSTEM_OPTION_STRING_RECORD_NUMBER 0x0000000A


//////////////////////////////////////////////////////////////////////////////
//
// Misc. Number of Installable Languages - SMBIOS Type 13
//
#define EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES_RECORD_NUMBER 0x0000000B

typedef struct {
  UINT32                              AbbreviatedLanguageFormat :1;
  UINT32                              Reserved                  :31;
} EFI_MISC_LANGUAGE_FLAGS;

typedef struct {
  UINT16                              NumberOfInstallableLanguages;
  EFI_MISC_LANGUAGE_FLAGS             LanguageFlags;
  UINT16                              CurrentLanguageNumber;
} EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES;       


//////////////////////////////////////////////////////////////////////////////
//
// Misc. System Language String
//
#define EFI_MISC_SYSTEM_LANGUAGE_STRING_RECORD_NUMBER 0x0000000C

typedef struct {
  UINT16                              LanguageId;
  STRING_REF                          SystemLanguageString;
} EFI_MISC_SYSTEM_LANGUAGE_STRING;      


//////////////////////////////////////////////////////////////////////////////
//
//  Misc. Pointing Device Type - SMBIOS Type 21
//
#define EFI_MISC_POINTING_DEVICE_TYPE_RECORD_NUMBER 0x0000000F

typedef enum { 
  EfiPointingDeviceTypeOther = 1,
  EfiPointingDeviceTypeUnknown = 2,
  EfiPointingDeviceTypeMouse = 3,
  EfiPointingDeviceTypeTrackBall = 4,
  EfiPointingDeviceTypeTrackPoint = 5,
  EfiPointingDeviceTypeGlidePoint = 6,
  EfiPointingDeviceTouchPad = 7,
  EfiPointingDeviceTouchScreen = 8,
  EfiPointingDeviceOpticalSensor = 9,
} EFI_MISC_POINTING_DEVICE_TYPE;

typedef enum {  
  EfiPointingDeviceInterfaceOther = 1,
  EfiPointingDeviceInterfaceUnknown = 2,
  EfiPointingDeviceInterfaceSerial = 3,
  EfiPointingDeviceInterfacePs2 = 4,
  EfiPointingDeviceInterfaceInfrared = 5,
  EfiPointingDeviceInterfaceHpHil = 6,
  EfiPointingDeviceInterfaceBusMouse = 7,
  EfiPointingDeviceInterfaceADB = 8,
  EfiPointingDeviceInterfaceBusMouseDB9 = 0xA0,
  EfiPointingDeviceInterfaceBusMouseMicroDin = 0xA1,
  EfiPointingDeviceInterfaceUsb = 0xA2,
} EFI_MISC_POINTING_DEVICE_INTERFACE;

typedef struct {
  EFI_MISC_POINTING_DEVICE_TYPE       PointingDeviceType;
  EFI_MISC_POINTING_DEVICE_INTERFACE  PointingDeviceInterface;
  UINT16                              NumberPointingDeviceButtons;
  EFI_DEVICE_PATH_PROTOCOL            PointingDevicePath;
} EFI_MISC_ONBOARD_DEVICE_TYPE_DATA;      


//////////////////////////////////////////////////////////////////////////////
//
// Misc. Reset Capabilities - SMBIOS Type 23
//
#define EFI_MISC_RESET_CAPABILITIES_RECORD_NUMBER 0x00000011

typedef struct {
  UINT32  Status              :1;
  UINT32  BootOption          :2;
  UINT32  BootOptionOnLimit   :2;
  UINT32  WatchdogTimerPresent:1;
  UINT32  Reserved            :26;
} EFI_MISC_RESET_CAPABILITIES_TYPE;

typedef struct {
  EFI_MISC_RESET_CAPABILITIES_TYPE  ResetCapabilities;
  UINT16                            ResetCount;
  UINT16                            ResetLimit;
  UINT16                            ResetTimerInterval;
  UINT16                            ResetTimeout;
} EFI_MISC_RESET_CAPABILITIES;
 
typedef struct {
    EFI_MISC_RESET_CAPABILITIES     ResetCapabilities;
    UINT16                        ResetCount;
    UINT16                        ResetLimit;
    UINT16                        ResetTimerInterval;
    UINT16                        ResetTimeout;
} EFI_MISC_RESET_CAPABILITIES_DATA;       

//////////////////////////////////////////////////////////////////////////////
//
// Misc. Hardware Security - SMBIOS Type 24
//
#define EFI_MISC_HARDWARE_SECURITY_RECORD_NUMBER 0x00000012

typedef enum {
  EfiHardwareSecurityStatusDisabled = 0,
  EfiHardwareSecurityStatusEnabled = 1,
  EfiHardwareSecurityStatusNotImplemented = 2,
  EfiHardwareSecurityStatusUnknown = 3
} EFI_MISC_HARDWARE_SECURITY_STATUS; 

typedef struct {
  EFI_MISC_HARDWARE_SECURITY_STATUS   FrontPanelResetStatus   :2;  
  EFI_MISC_HARDWARE_SECURITY_STATUS   AdministratorPasswordStatus   :2;  
  EFI_MISC_HARDWARE_SECURITY_STATUS   KeyboardPasswordStatus :2;  
  EFI_MISC_HARDWARE_SECURITY_STATUS   PowerOnPasswordStatus :2;  
  EFI_MISC_HARDWARE_SECURITY_STATUS    Reserved :24;  
} EFI_MISC_HARDWARE_SECURITY_SETTINGS;

typedef struct {
  EFI_MISC_HARDWARE_SECURITY_SETTINGS HardwareSecuritySettings;
} EFI_MISC_HARDWARE_SECURITY_SETTINGS_DATA;       

//////////////////////////////////////////////////////////////////////////////
//
// Misc. BIS Entry Point - SMBIOS Type 31
//
#define EFI_MISC_BIS_ENTRY_POINT_RECORD_NUMBER          0x00000019

typedef struct {
  EFI_PHYSICAL_ADDRESS       BisEntryPoint;
} EFI_MISC_BIS_ENTRY_POINT;    

//////////////////////////////////////////////////////////////////////////////
//
// Misc. Boot Information - SMBIOS Type 32
//
#define EFI_MISC_BOOT_INFORMATION_STATUS_RECORD_NUMBER  0x0000001A

typedef enum {  
  EfiBootInformationStatusNoError = 0,
  EfiBootInformationStatusNoBootableMedia = 1,
  EfiBootInformationStatusNormalOSFailedLoading = 2,
  EfiBootInformationStatusFirmwareDetectedFailure = 3,
  EfiBootInformationStatusOSDetectedFailure = 4,
  EfiBootInformationStatusUserRequestedBoot = 5,
  EfiBootInformationStatusSystemSecurityViolation = 6,
  EfiBootInformationStatusPreviousRequestedImage = 7,
  EfiBootInformationStatusWatchdogTimerExpired = 8,
  EfiBootInformationStatusStartReserved = 9,
  EfiBootInformationStatusStartOemSpecific = 128,
  EfiBootInformationStatusStartProductSpecific = 192,
} EFI_MISC_BOOT_INFORMATION_STATUS_TYPE;

typedef struct {
    EFI_MISC_BOOT_INFORMATION_STATUS_TYPE BootInformationStatus;
    UINT8                                 BootInformationData[9];
} EFI_MISC_BOOT_INFORMATION_STATUS;

//////////////////////////////////////////////////////////////////////////////
//
// IPMI Data Record - SMBIOS Type 38
//
typedef enum {  
  EfiIpmiOther = 0,
  EfiIpmiKcs = 1,
  EfiIpmiSmic = 2,
  EfiIpmiBt = 3,
} EFI_MISC_IPMI_INTERFACE_TYPE;

typedef struct {
  UINT16  IpmiSpecLeastSignificantDigit:4;
  UINT16  IpmiSpecMostSignificantDigit:4;
  UINT16  Reserved:8;
} EFI_MISC_IPMI_SPECIFICATION_REVISION;

typedef struct {
  EFI_MISC_IPMI_INTERFACE_TYPE          IpmiInterfaceType;
  EFI_MISC_IPMI_SPECIFICATION_REVISION  IpmiSpecificationRevision;
  UINT16                                IpmiI2CSlaveAddress;
  UINT16                                IpmiNvDeviceAddress;
  UINT64                                IpmiBaseAddress;
  EFI_DEVICE_PATH_PROTOCOL              IpmiDevicePath;
} EFI_MISC_IPMI_INTERFACE_TYPE_DATA;
       
#define EFI_MISC_IPMI_INTERFACE_TYPE_RECORD_NUMBER  0x0000001D

//////////////////////////////////////////////////////////////////////////////
//
//System Power supply Record - SMBIOS Type 39
//
typedef struct {
  UINT16  PowerSupplyHotReplaceable  :1;
  UINT16  PowerSupplyPresent         :1;
  UINT16  PowerSupplyUnplugged       :1;
  UINT16  InputVoltageRangeSwitch    :4;
  UINT16  PowerSupplyStatus           :3;
  UINT16  PowerSupplyType             :4;
  UINT16  Reserved                   :2;
} POWER_SUPPLY_CHARACTERISTICS;

typedef struct {
  UINT16                          PowerUnitGroup;
  STRING_REF                      PowerSupplyLocation;
  STRING_REF                      PowerSupplyDeviceName;
  STRING_REF                      PowerSupplyManufacturer;
  STRING_REF                      PowerSupplySerialNumber;
  STRING_REF                      PowerSupplyAssetTagNumber;
  STRING_REF                      PowerSupplyModelPartNumber;
  STRING_REF                      PowerSupplyRevisionLevel;
  UINT16                          PowerSupplyMaxPowerCapacity;
  POWER_SUPPLY_CHARACTERISTICS    PowerSupplyCharacteristics;
  EFI_INTER_LINK_DATA             PowerSupplyInputVoltageProbeLink;
  EFI_INTER_LINK_DATA             PowerSupplyCoolingDeviceLink;
  EFI_INTER_LINK_DATA             PowerSupplyInputCurrentProbeLink;
} EFI_MISC_SYSTEM_POWER_SUPPLY;
#define EFI_MISC_SYSTEM_POWER_SUPPLY_RECORD_NUMBER 0x0000001E

//////////////////////////////////////////////////////////////////////////////
//
// OEM Data Record - SMBIOS Type 0x80-0xFF
//
typedef struct {
  UINT8       Type;
  UINT8       Length;
  UINT16      Handle;
} SMBIOS_STRUCTURE_HDR;

typedef struct {
  SMBIOS_STRUCTURE_HDR          Header;
  UINT8                         RawData[1];
} EFI_MISC_SMBIOS_STRUCT_ENCAPSULATION; 

#define EFI_MISC_SMBIOS_STRUCT_ENCAP_RECORD_NUMBER  0x0000001F 

//////////////////////////////////////////////////////////////////////////////
//
//
//
typedef union {
  EFI_MISC_LAST_PCI_BUS                         LastPciBus;
  EFI_MISC_BIOS_VENDOR                          MiscBiosVendor;
  EFI_MISC_SYSTEM_MANUFACTURER                  MiscSystemManufacturer;
  EFI_MISC_BASE_BOARD_MANUFACTURER              MiscBaseBoardManufacturer;
  EFI_MISC_CHASSIS_MANUFACTURER                 MiscChassisManufacturer;  
  EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR   MiscPortInternalConnectorDesignator;
  EFI_MISC_SYSTEM_SLOT_DESIGNATION              MiscSystemSlotDesignation;
  EFI_MISC_ONBOARD_DEVICE                       MiscOnboardDevice;
  EFI_MISC_OEM_STRING                           MiscOemString;
  EFI_MISC_SYSTEM_OPTION_STRING                 MiscOptionString;
  EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES      NumberOfInstallableLanguages;
  EFI_MISC_SYSTEM_LANGUAGE_STRING               MiscSystemLanguageString;  
  EFI_MISC_ONBOARD_DEVICE_TYPE_DATA             MiscOnboardDeviceTypeData;
  EFI_MISC_RESET_CAPABILITIES_DATA              MiscResetCapablilitiesData;
  EFI_MISC_HARDWARE_SECURITY_SETTINGS_DATA      MiscHardwareSecuritySettingsData;  
  EFI_MISC_BIS_ENTRY_POINT                      MiscBisEntryPoint;
  EFI_MISC_BOOT_INFORMATION_STATUS              MiscBootInformationStatus;
  EFI_MISC_IPMI_INTERFACE_TYPE_DATA             MiscIpmiInterfaceTypeData;
  EFI_MISC_SYSTEM_POWER_SUPPLY                  MiscPowerSupplyInfo;
  EFI_MISC_SMBIOS_STRUCT_ENCAPSULATION          MiscSmbiosStructEncapsulation;  
} EFI_MISC_SUBCLASS_RECORDS;

//
//
//
typedef struct {
  EFI_SUBCLASS_TYPE1_HEADER       Header;
  EFI_MISC_SUBCLASS_RECORDS       Record;
} EFI_MISC_SUBCLASS_DRIVER_DATA;

#pragma pack()

#endif /* _DATAHUB_SUBCLASS_MISC_H_ */
/* eof - DataHubSubClassMisc.h */