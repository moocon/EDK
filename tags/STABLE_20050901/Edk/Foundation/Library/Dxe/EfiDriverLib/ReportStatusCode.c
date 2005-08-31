/*++

Copyright 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ReportStatusCode.c

Abstract:

--*/

#include "Tiano.h"
#include "EfiDriverLib.h"
#include EFI_PROTOCOL_DEFINITION (DevicePath)
#include EFI_GUID_DEFINITION (StatusCodeDataTypeId)

EFI_STATUS
ReportStatusCodeWithDevicePath (
  IN EFI_STATUS_CODE_TYPE     Type,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 * CallerId OPTIONAL,
  IN EFI_DEVICE_PATH_PROTOCOL * DevicePath
  )
/*++

Routine Description:

  Report device path through status code.

Arguments:

  Type        - Code type
  Value       - Code value
  Instance    - Instance number
  CallerId    - Caller name
  DevicePath  - Device path that to be reported

Returns:

  Status code.

  EFI_OUT_OF_RESOURCES - No enough buffer could be allocated

--*/
{
  UINT16                    Size;
  UINT16                    DevicePathSize;
  EFI_STATUS_CODE_DATA      *ExtendedData;
  EFI_DEVICE_PATH_PROTOCOL  *ExtendedDevicePath;
  EFI_STATUS                Status;

  DevicePathSize  = (UINT16) EfiDevicePathSize (DevicePath);
  Size            = DevicePathSize + sizeof (EFI_STATUS_CODE_DATA);
  ExtendedData    = (EFI_STATUS_CODE_DATA *) EfiLibAllocatePool (Size);
  if (ExtendedData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ExtendedDevicePath = EfiConstructStatusCodeData (Size, &gEfiStatusCodeSpecificDataGuid, ExtendedData);
  EfiCopyMem (ExtendedDevicePath, DevicePath, DevicePathSize);

  Status = gRT->ReportStatusCode (Type, Value, Instance, CallerId, (EFI_STATUS_CODE_DATA *) ExtendedData);

  gBS->FreePool (ExtendedData);
  return Status;
}
