/*++

Copyright (c) 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

Module Name:

  IScsiExtScsiPassThru.h

Abstract:

--*/

#ifndef _ISCSI_EXT_SCSI_PASS_THRU_H_
#define _ISCSI_EXT_SCSI_PASS_THRU_H_

#include EFI_PROTOCOL_CONSUMER (ScsiPassThruExt)

extern EFI_EXT_SCSI_PASS_THRU_PROTOCOL  gIScsiExtScsiPassThruProtocolTemplate;

#endif
