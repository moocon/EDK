/*++
Copyright 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  IfrOpCodeCreation.c

Abstract:

  Library Routines to create IFR independent of string data - assume tokens already exist
  Primarily to be used for exporting op-codes at a label in pre-defined forms.

Revision History:

--*/

#include "IfrLibrary.h"

EFI_STATUS
CreateSubTitleOpCode (
  IN      STRING_REF          StringToken,
  IN OUT  VOID                *FormBuffer
  )
/*++

Routine Description:

  Create a SubTitle opcode independent of string creation
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)
  
Arguments:
  
Returns: 

--*/
{
  EFI_IFR_SUBTITLE            Subtitle;

  Subtitle.Header.OpCode = EFI_IFR_SUBTITLE_OP;
  Subtitle.Header.Length = sizeof(EFI_IFR_SUBTITLE);
  Subtitle.SubTitle = StringToken;

  EfiCopyMem (FormBuffer, &Subtitle, sizeof (EFI_IFR_SUBTITLE));
  return EFI_SUCCESS;
}


EFI_STATUS
CreateTextOpCode (
  IN      STRING_REF          StringToken,
  IN      STRING_REF          StringTokenTwo,
  IN      STRING_REF          StringTokenThree,
  IN      UINT8               Flags,
  IN      UINT16              Key,
  IN OUT  VOID                *FormBuffer
  )
/*++

Routine Description:

  Create a Text opcode independent of string creation
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)
  
Arguments:
  
Returns: 

--*/
{
  EFI_IFR_TEXT                Text;

  Text.Header.OpCode = EFI_IFR_TEXT_OP;
  Text.Header.Length = sizeof(EFI_IFR_TEXT);
  Text.Text = StringToken;

  Text.TextTwo = StringTokenTwo;
  Text.Help = StringTokenThree;
  Text.Flags = Flags;
  Text.Key = Key;

  EfiCopyMem(FormBuffer, &Text, sizeof (EFI_IFR_TEXT));

  return EFI_SUCCESS;
}


EFI_STATUS
CreateGotoOpCode (
  IN      UINT16              FormId,
  IN      STRING_REF          StringToken,
  IN      STRING_REF          StringTokenTwo,
  IN      UINT8               Flags,
  IN      UINT16              Key,
  IN OUT  VOID                *FormBuffer
  )
/*++

Routine Description:

  Create a hyperlink opcode independent of string creation
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)
  
Arguments:
  
Returns: 

--*/
{
  EFI_IFR_REF                 Hyperlink;

  Hyperlink.Header.OpCode = EFI_IFR_REF_OP;
  Hyperlink.Header.Length = sizeof(EFI_IFR_REF);
  Hyperlink.FormId = FormId;
  Hyperlink.Prompt = StringToken;
  Hyperlink.Help = StringTokenTwo;
  Hyperlink.Key = Key;
  Hyperlink.Flags = Flags;

  EfiCopyMem(FormBuffer, &Hyperlink, sizeof (EFI_IFR_REF));

  return EFI_SUCCESS;
}


EFI_STATUS
CreateOneOfOpCode (
  IN      UINT16              QuestionId,
  IN      UINT8               DataWidth,
  IN      STRING_REF          PromptToken,
  IN      STRING_REF          HelpToken,
  IN      IFR_OPTION          *OptionsList,
  IN      UINTN               OptionCount,
  IN OUT  VOID                *FormBuffer
  )
/*++

Routine Description:

  Create a one-of opcode with a set of option op-codes to choose from independent of string creation.
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)

  OptionsList is a pointer to a null-terminated list of option descriptions.  Ensure that OptionsList[x].StringToken
  has been filled in since this routine will not generate StringToken values.
  
Arguments:
  
Returns: 

--*/
{
  UINTN                       Index;
  EFI_IFR_ONE_OF              OneOf;
  EFI_IFR_ONE_OF_OPTION       OneOfOption;
  EFI_IFR_END_ONE_OF          EndOneOf;
  UINT8                       *LocalBuffer;

  //
  // We do not create op-code storage widths for one-of in excess of 16 bits for now
  //
  if (DataWidth > 2) {
    return EFI_DEVICE_ERROR;
  }

  OneOf.Header.OpCode = EFI_IFR_ONE_OF_OP;
  OneOf.Header.Length = sizeof(EFI_IFR_ONE_OF);
  OneOf.QuestionId = QuestionId;
  OneOf.Width = DataWidth;
  OneOf.Prompt = PromptToken;

  OneOf.Help = HelpToken;

  LocalBuffer = (CHAR8 *)FormBuffer;

  EfiCopyMem(LocalBuffer, &OneOf, sizeof (EFI_IFR_ONE_OF));

  LocalBuffer = (CHAR8 *)(LocalBuffer + sizeof (EFI_IFR_ONE_OF));

  for (Index = 0; Index < OptionCount; Index++) {
    OneOfOption.Header.OpCode = EFI_IFR_ONE_OF_OPTION_OP;
    OneOfOption.Header.Length = sizeof (EFI_IFR_ONE_OF_OPTION);

    OneOfOption.Option = OptionsList[Index].StringToken;
    OneOfOption.Value = OptionsList[Index].Value;
    OneOfOption.Flags = OptionsList[Index].Flags;
    OneOfOption.Key = OptionsList[Index].Key;

    EfiCopyMem(LocalBuffer, &OneOfOption, sizeof (EFI_IFR_ONE_OF_OPTION));

    LocalBuffer = (CHAR8 *)(LocalBuffer + sizeof (EFI_IFR_ONE_OF_OPTION));
  }

  EndOneOf.Header.Length = sizeof(EFI_IFR_END_ONE_OF);
  EndOneOf.Header.OpCode = EFI_IFR_END_ONE_OF_OP;

  EfiCopyMem(LocalBuffer, &EndOneOf, sizeof (EFI_IFR_END_ONE_OF));

  LocalBuffer = (CHAR8 *)(LocalBuffer + sizeof (EFI_IFR_END_ONE_OF));

  return EFI_SUCCESS;
}

EFI_STATUS
CreateOrderedListOpCode (
  IN      UINT16              QuestionId,
  IN      UINT8               MaxEntries,
  IN      STRING_REF          PromptToken,
  IN      STRING_REF          HelpToken,
  IN      IFR_OPTION          *OptionsList,
  IN      UINTN               OptionCount,
  IN OUT  VOID                *FormBuffer
  )
/*++

Routine Description:

  Create a ordered list opcode with a set of option op-codes to choose from independent of string creation.
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)

  OptionsList is a pointer to a null-terminated list of option descriptions.  Ensure that OptionsList[x].StringToken
  has been filled in since this routine will not generate StringToken values.
  
Arguments:
  
Returns: 

--*/
{
  UINTN                       Index;
  EFI_IFR_ORDERED_LIST        OrderedList;
  EFI_IFR_ONE_OF_OPTION       OrderedListOption;
  EFI_IFR_END_ONE_OF          EndOrderedList;
  UINT8                       *LocalBuffer;

  OrderedList.Header.OpCode = EFI_IFR_ORDERED_LIST_OP;
  OrderedList.Header.Length = sizeof(EFI_IFR_ORDERED_LIST);
  OrderedList.QuestionId = QuestionId;
  OrderedList.MaxEntries = MaxEntries;
  OrderedList.Prompt = PromptToken;

  OrderedList.Help = HelpToken;

  LocalBuffer = (CHAR8 *)FormBuffer;

  EfiCopyMem(LocalBuffer, &OrderedList, sizeof (EFI_IFR_ORDERED_LIST));

  LocalBuffer = (CHAR8 *)(LocalBuffer + sizeof (EFI_IFR_ORDERED_LIST));

  for (Index = 0; Index < OptionCount; Index++) {
    OrderedListOption.Header.OpCode = EFI_IFR_ONE_OF_OPTION_OP;
    OrderedListOption.Header.Length = sizeof (EFI_IFR_ONE_OF_OPTION);

    OrderedListOption.Option = OptionsList[Index].StringToken;
    OrderedListOption.Value = OptionsList[Index].Value;
    OrderedListOption.Flags = OptionsList[Index].Flags;
    OrderedListOption.Key = OptionsList[Index].Key;

    EfiCopyMem(LocalBuffer, &OrderedListOption, sizeof (EFI_IFR_ONE_OF_OPTION));

    LocalBuffer = (CHAR8 *)(LocalBuffer + sizeof (EFI_IFR_ONE_OF_OPTION));
  }

  EndOrderedList.Header.Length = sizeof(EFI_IFR_END_ONE_OF);
  EndOrderedList.Header.OpCode = EFI_IFR_END_ONE_OF_OP;

  EfiCopyMem(LocalBuffer, &EndOrderedList, sizeof (EFI_IFR_END_ONE_OF));

  LocalBuffer = (CHAR8 *)(LocalBuffer + sizeof (EFI_IFR_END_ONE_OF));

  return EFI_SUCCESS;
}

EFI_STATUS
CreateCheckBoxOpCode (
  IN      UINT16              QuestionId,
  IN      UINT8               DataWidth,
  IN      STRING_REF          PromptToken,
  IN      STRING_REF          HelpToken,
  IN      UINT8               Flags,
  IN      UINT16              Key,
  IN OUT  VOID                *FormBuffer
  )
/*++

Routine Description:

  Create a checkbox opcode independent of string creation
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)
  
Arguments:
  
Returns: 

--*/
{
  EFI_IFR_CHECK_BOX           CheckBox;

  //
  // We do not create op-code storage widths for checkbox in excess of 8 bits for now
  //
  if (DataWidth > 1) {
    return EFI_DEVICE_ERROR;
  }

  CheckBox.Header.OpCode = EFI_IFR_CHECKBOX_OP;
  CheckBox.Header.Length = sizeof(EFI_IFR_CHECK_BOX);
  CheckBox.QuestionId = QuestionId;
  CheckBox.Width = DataWidth;
  CheckBox.Prompt = PromptToken;

  CheckBox.Help = HelpToken;
  CheckBox.Flags = Flags;
  CheckBox.Key = Key;

  EfiCopyMem(FormBuffer, &CheckBox, sizeof (EFI_IFR_CHECK_BOX));

  return EFI_SUCCESS;
}


EFI_STATUS
CreateNumericOpCode (
  IN      UINT16              QuestionId,
  IN      UINT8               DataWidth,
  IN      STRING_REF          PromptToken,
  IN      STRING_REF          HelpToken,
  IN      UINT16              Minimum,
  IN      UINT16              Maximum,
  IN      UINT16              Step,
  IN      UINT16              Default,
  IN      UINT8               Flags,
  IN      UINT16              Key,
  IN OUT  VOID                *FormBuffer
  )
/*++

Routine Description:

  Create a numeric opcode independent of string creation
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)
  
Arguments:
  
Returns: 

--*/
{
  EFI_IFR_NUMERIC             Numeric;

  //
  // We do not create op-code storage widths for numerics in excess of 16 bits for now
  //
  if (DataWidth > 2) {
    return EFI_DEVICE_ERROR;
  }

  Numeric.Header.OpCode = EFI_IFR_NUMERIC_OP;
  Numeric.Header.Length = sizeof(EFI_IFR_NUMERIC);
  Numeric.QuestionId = QuestionId;
  Numeric.Width = DataWidth;
  Numeric.Prompt = PromptToken;

  Numeric.Help = HelpToken;
  Numeric.Minimum = Minimum;
  Numeric.Maximum = Maximum;
  Numeric.Step = Step;
  Numeric.Default = Default;
  Numeric.Flags = Flags;
  Numeric.Key = Key;

  EfiCopyMem(FormBuffer, &Numeric, sizeof (EFI_IFR_NUMERIC));

  return EFI_SUCCESS;
}


EFI_STATUS
CreateStringOpCode (
  IN      UINT16              QuestionId,
  IN      UINT8               DataWidth,
  IN      STRING_REF          PromptToken,
  IN      STRING_REF          HelpToken,
  IN      UINT8               MinSize,
  IN      UINT8               MaxSize,
  IN      UINT8               Flags,
  IN      UINT16              Key,
  IN OUT  VOID                *FormBuffer
  )
/*++

Routine Description:

  Create a numeric opcode independent of string creation
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)
  
Arguments:
  
Returns: 

--*/
{
  EFI_IFR_STRING              String;

  String.Header.OpCode = EFI_IFR_STRING_OP;
  String.Header.Length = sizeof(EFI_IFR_STRING);
  String.QuestionId = QuestionId;
  String.Width = DataWidth;
  String.Prompt = PromptToken;

  String.Help = HelpToken;
  String.MinSize = MinSize;
  String.MaxSize = MaxSize;
  String.Flags = Flags;
  String.Key = Key;

  EfiCopyMem(FormBuffer, &String, sizeof (EFI_IFR_STRING));

  return EFI_SUCCESS;
}


EFI_STATUS
CreateBannerOpCode (
  IN      UINT16              Title,
  IN      UINT16              LineNumber,
  IN      UINT8               Alignment,
  IN OUT  VOID                *FormBuffer
  )
/*++

Routine Description:

  Create a banner opcode.  This is primarily used by the FrontPage implementation from BDS.
  
Arguments:
  
Returns: 

--*/
{
  EFI_IFR_BANNER              Banner;

  Banner.Header.OpCode = EFI_IFR_BANNER_OP;
  Banner.Header.Length = sizeof(EFI_IFR_BANNER);
  EfiCopyMem(&Banner.Title, &Title, sizeof (UINT16));
  EfiCopyMem(&Banner.LineNumber, &LineNumber, sizeof (UINT16));
  Banner.Alignment = Alignment;

  EfiCopyMem(FormBuffer, &Banner, sizeof (EFI_IFR_BANNER));

  return EFI_SUCCESS;
}

