/*++

Copyright 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ScsiIo.c

Abstract:

  SCSI I/O protocol.

--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION (ScsiIo)

EFI_GUID  gEfiScsiIoProtocolGuid = EFI_SCSI_IO_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiScsiIoProtocolGuid, "SCSI IO Protocol", "EFI SCSI IO protocol");