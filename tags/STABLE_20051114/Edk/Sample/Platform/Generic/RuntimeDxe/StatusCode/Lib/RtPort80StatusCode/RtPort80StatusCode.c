/*++

Copyright (c) 2004 - 2005, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  RtPort80StatusCode.c
   
Abstract:

  Lib to provide port 80 status code reporting Routines. This routine
  does not use PPI's but is monolithic.

  In general you should use PPI's, but some times a monolithic driver
  is better. The best justification for monolithic code is debug.

--*/

#include "RtPort80StatusCode.h"

EFI_STATUS
RtPort80ReportStatusCode (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 * CallerId,
  IN EFI_STATUS_CODE_DATA     * Data OPTIONAL
  )
/*++

Routine Description:

  Provide a port 80 status code

Arguments:

  Same as ReportStatusCode PPI
    
Returns:

  EFI_SUCCESS   Always returns success.

--*/
{
  UINT8 Port80Code;

  //
  // Progress or error code, Output Port 80h card
  //
  if (CodeTypeToPostCode (CodeType, Value, &Port80Code)) {
    IoWrite8 (0x80, Port80Code);
  }

  if (((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_PROGRESS_CODE) ||
      ((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_ERROR_CODE) ||
      ((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_DEBUG_CODE)
      ) {
    IoWrite8 (0x81, (UINT8) CodeType);
  }

  return EFI_SUCCESS;
}