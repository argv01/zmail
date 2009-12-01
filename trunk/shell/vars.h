/* vars.h	   Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef _ZM_VARS_H_
#define _ZM_VARS_H_

#ifndef OSCONFIG
#include <osconfig.h>
#endif /* OSCONFIG */

#include "config/features.h"
#include "linklist.h"
#include "zctype.h"
#include "zccmac.h"

#ifdef GUI
extern int autosave_ct;
#endif /* GUI */

/* Flags for variable type and applicability information. */

/* V_STRING should be removed from V_PATHLIST and V_WORDLIST when those
 * types are fully implemented.
 */

#define V_TTY		ULBIT(0)
#define V_CURSES	ULBIT(1)
#define V_GUI		ULBIT(2)
#define V_READONLY	ULBIT(3)
#define V_BOOLEAN	ULBIT(4)
#define V_STRING	ULBIT(5)
#define V_MULTIVAL	ULBIT(6)
#define V_NUMERIC	ULBIT(7)
#define V_PERMANENT	ULBIT(8)
#define V_PATHLIST	(ULBIT(9)|V_STRING)	/* dir[: ]dir[: ]dir */
#define V_WORDLIST	(ULBIT(10)|V_STRING)	/* word[, ]word[, ]word */
#define V_ARRAY		ULBIT(11)
#define V_VUI		ULBIT(12)		/* "lite" mode */
#define V_SINGLEVAL	ULBIT(13)
#define V_ADMIN		ULBIT(14)		/* only admin can set */
#define V_MAC		ULBIT(15)
#define V_MSWINDOWS	ULBIT(16)

/* this struct goes away */
typedef struct {
    char *v_label;		/* prompt or name of multival field */
    char *v_description;	/* summary of variable's lot in life */
    long v_desc_pos;		/* seek position of explanatory text */
} VarItem;

typedef struct Variable {
    struct link v_link;
#define v_opt v_link.l_name	/* the actual name of the variable */
    u_long v_flags;		/* kind of variable and usage context */
    char *v_default;		/* the default value, if any */
    unsigned int v_num_vals;	/* number of entries ref'd by v_values */
    /* v_values should be of type pointer, num_vals is size of this array */
    VarItem *v_values;		/* permissible values (for multivalue) */
    VarItem v_prompt;		/* prompt plus description */
    int v_gui_max;		/* maximum value for variables dialog */
    unsigned long v_category;	/* category number for grouping */
} Variable;

extern Variable *variables;
extern int n_variables;

/* maximum number of multivalue values (used in GUI).  If you change
 * this, you'll have to fix the geometry of the variables dialog.
 */
#define MAX_MULTIVALS 16

/*
 * Index mapping type for Z-Mail shell variables.  When a variable is added
 * to the shell, three entries must be made:  one in this enum, one in the
 * list of #defines that map into var_table, and one in var_table itself.
 * This enum should be kept alphabetically sorted except for VT_Unknown.
 */
typedef enum {
    VT_Unknown,		/* Filler for 0th slot */
    VT_AddressBook,
    VT_AddressCache,
    VT_AddressCheck,
    VT_AddressFilter,	/* UGH */
    VT_AddressSort,
#ifdef ZPOP_SYNC
    VT_Afterlife,
#endif /* ZPOP_SYNC */
    VT_Alternates,
    VT_Alwaysexpand,
    VT_Alwaysignore,
    VT_AlwaysSendSpooled,
    VT_AlwaysShowTaskm,
    VT_AlwaysSort,
    VT_Ask,
    VT_Askbcc,
    VT_Askcc,
    VT_Asksub,
    VT_AttachLabel,
    VT_AttachPrune,
    VT_AttachTypes,
    VT_AutoRoute,
    VT_Autoclear,
    VT_Autodismiss,
    VT_Autodisplay,
    VT_Autoedit,
    VT_Autoformat,
    VT_Autoiconify,
    VT_Autoinclude,
    VT_Autopinup,
    VT_Autoprint,
    VT_AutosaveCount,
    VT_Autosend,
    VT_Autosign,
    VT_Autosign2,
    VT_Autotyper,
    VT_Baud,
    VT_BlinkTime,
    VT_Cdpath,
    VT_Child,
    VT_CmdHelp,
    VT_ColorsDB,
    VT_CompAttachLabel,
    VT_CompStatusBarFmt,
    VT_Complete,
    VT_ComposeLines,
    VT_ComposePanes,
    VT_Connected,
    VT_ConnectType,
    VT_Crt,
    VT_CrtWin,
    VT_Cwd,
    VT_DateReceived,
    VT_Dead,
    VT_Deletesave,
    VT_DetachDir,
    VT_DisplayCharset,
    VT_DisplayHeaders,
    VT_DomainRoute,
    VT_Dot,
    VT_DotLock,
    VT_EditHdrs,
    VT_Editor,
    VT_Escape,
    VT_ExitSaveopts,
    VT_ExpireCache,
    VT_ExtSummaryFmt,
    VT_FetchTimeout,
    VT_Fignore,
    VT_Filec,
    VT_FileCharset,
    VT_FilelistFmt,
    VT_FileTest,
    VT_FirstPartPrimary,
    VT_FkeyLabels,
    VT_Folder,
    VT_FolderTitle,
    VT_FolderType,
    VT_FontsDB,
    VT_Fortunates,
    VT_Fortune,
    VT_FromAddress,
    VT_FullscreenHelp,
    VT_FunctionHelp,
    VT_GuiHelp,
    VT_Hangup,
    VT_HeaderEncoding,
    VT_HdrFormat,
    VT_Hidden,
    VT_History,
    VT_Hold,
    VT_Home,
    VT_Hostname,
    VT_IgnoreBang,
    VT_Ignoreeof,
#if defined( IMAP )
    VT_ImapCache,
    VT_ImapShared,
    VT_ImapSynchronize,
    VT_ImapTimeout,
    VT_ImapUser,
    VT_ImapNoPrompt,
#endif
    VT_InReplyTo,
    VT_IncomingCharset,
    VT_IndentStr,
    VT_IndexDir,
    VT_IndexSize,
    VT_IntrLevel,
    VT_Ispeller,
    VT_Keepsave,
    VT_KnownHosts,
    VT_LayoutDB,
    VT_LdapService,
    VT_Logfile,
    VT_LookupCharset,
    VT_LookupFile,
    VT_LookupHostIp,
    VT_LookupMax,
    VT_LookupMode,
    VT_LookupSep,
    VT_LookupService,
    VT_Mailhost,
    VT_MailIcon,
    VT_MailQueue,
    VT_MailboxName,
    VT_MainFolderTitle,
    VT_MainPanes,
    VT_MainStatusBarFmt,
    VT_MaxTextLength,
    VT_Mbox,
    VT_MessageField,
    VT_MessageLines,
    VT_MessagePanes,
    VT_Metoo,
    VT_MilTime,
    VT_MsgAttachLabel,
    VT_MsgSeparator,
    VT_MsgStatusBarFmt,
    VT_MsgWin,
    VT_MsgWinHdrFmt,
    VT_Name,
    VT_Newline,
    VT_NewmailIcon,
    VT_NewmailScroll,
    VT_NoExpand,
    VT_NoHdrs,
    VT_NoReverse,
    VT_Nonobang,
    VT_Nosave,
    VT_Organization,
    VT_OutgoingCharset,
    VT_Output,
    VT_Pager,
    VT_PickyMta,
    VT_PopOptions,
    VT_PopTimeout,
    VT_PopUser,
    VT_PostIndentStr,
    VT_PreIndentStr,
    VT_Presign,
    VT_PrintCmd,
    VT_Printer,
    VT_PrinterCharset,
    VT_PrinterOpt,
    VT_Prompt,
    VT_Quiet,
    VT_Realname,
    VT_Record,
    VT_RecordControl,
    VT_RecordMax,
    VT_RecordUsers,
    VT_Recursive,
    VT_ReplyToHdr,
    VT_SaveEmpty,
    VT_Screen,
    VT_ScreenWin,
    VT_Scrollpct,
    VT_Sendmail,
    VT_Shell,
    VT_ShowDeleted,
    VT_Smtphost,
    VT_Sort,
    VT_Speller,
#ifdef PARTIAL_SEND
    VT_SplitLimit,
    VT_SplitSendmail,
    VT_SplitSize,
#endif /* PARTIAL_SEND */
    VT_Squeeze,
    VT_Status,
    VT_StatusBarFmt,
    VT_SummaryFmt,
    VT_SummaryLines,
    VT_Templates,
    VT_TextpartCharset,
    VT_Thisfolder,
    VT_Timeout,
    VT_Title,
    VT_Tmpdir,
#ifdef ZPOP_SYNC
    VT_Tombfile,
#endif /* ZPOP_SYNC */
    VT_ToolHelp,
    VT_Toplines,
    VT_TrustedFunctions,
    VT_Unix,
    VT_UseContentLength,
#if defined( IMAP )
    VT_UseImap,
#endif
    VT_UseLdap,
    VT_UsePop,
    VT_User,
    VT_UucpRoot,
    VT_Verbose,
    VT_Verify,
    VT_Visual,
    VT_Warning,
    VT_WindowShell,
    VT_Wineditor,
    VT_Winterm,
    VT_Wrap,
    VT_Wrapcolumn,
    VT_Zynchost,
    VT_ZyncOptions,
    VT_ZyncUser,
    VT_TotalIndices	/* Counter value, must be last */
} VT_Map;

/*
 * Structure for holding a variable entry.
 *
 * The vt_index field may be turn out to be extraneous but is useful
 * for debugging to make sure that the table matches the VT_Map enum.
 */

typedef struct {
    Variable	vt_default;
#define vt_link vt_default.v_link
#define vt_name vt_link.l_name
    VT_Map	vt_synonym;
#ifdef DEBUG
    VT_Map	vt_index;
#endif /* DEBUG */
} VT_Rec;

extern VT_Rec var_table[];

/*
 * Mapping from symbolic names to string names via the var_table
 */

#ifdef DEBUG
#define	VAR_MAP(x) \
	(var_table[(int)(x)].vt_index==(x)?var_table[(int)(x)].vt_name:NULL)
#define VIX(x)	, x
#else /* !DEBUG */
#define VAR_MAP(x) var_table[(int)(x)].vt_name
#define VIX(x)
#endif /* DEBUG */

#define VAR_REMAP(x) \
	(var_table[(int)(x)].vt_synonym != VT_Unknown ? \
	    VAR_MAP(var_table[(int)(x)].vt_synonym) : VAR_MAP(x))

#define VarUnknown		VAR_MAP(VT_Unknown)
#define VarAddressCache		VAR_MAP(VT_AddressCache)
#define VarAddressCheck		VAR_MAP(VT_AddressCheck)
#define VarAddressFilter	VAR_MAP(VT_AddressFilter)
#define VarAddressSort		VAR_MAP(VT_AddressSort)
#define VarAfterlife		VAR_MAP(VT_Afterlife)
#define VarAlternates		VAR_MAP(VT_Alternates)
#define VarAlwaysexpand		VAR_MAP(VT_Alwaysexpand)
#define VarAlwaysignore		VAR_MAP(VT_Alwaysignore)
#define VarAlwaysSendSpooled	VAR_MAP(VT_AlwaysSendSpooled)
#define VarAlwaysShowTaskm	VAR_MAP(VT_AlwaysShowTaskm)
#define VarAlwaysSort		VAR_MAP(VT_AlwaysSort)
#define VarAsk			VAR_MAP(VT_Ask)
#define VarAskbcc		VAR_MAP(VT_Askbcc)
#define VarAskcc		VAR_MAP(VT_Askcc)
#define VarAsksub		VAR_MAP(VT_Asksub)
#define VarAttachLabel		VAR_MAP(VT_AttachLabel)
#define VarAttachPrune		VAR_MAP(VT_AttachPrune)
#define VarAttachTypes		VAR_MAP(VT_AttachTypes)
#define VarAutoRoute		VAR_MAP(VT_AutoRoute)
#define VarAutoclear		VAR_MAP(VT_Autoclear)
#define VarAutodismiss		VAR_MAP(VT_Autodismiss)
#define VarAutodisplay		VAR_MAP(VT_Autodisplay)
#define VarAutoedit		VAR_MAP(VT_Autoedit)
#define VarAutoformat		VAR_MAP(VT_Autoformat)
#define VarAutoiconify		VAR_MAP(VT_Autoiconify)
#define VarAutoinclude		VAR_MAP(VT_Autoinclude)
#define VarAutopinup		VAR_MAP(VT_Autopinup)
#define VarAutoprint		VAR_MAP(VT_Autoprint)
#define VarAutosave		VAR_MAP(VT_AutosaveCount)
#define VarAutosend		VAR_MAP(VT_Autosend)
#define VarAutosign		VAR_MAP(VT_Autosign)
#define VarAutosign2		VAR_MAP(VT_Autosign2)
#define VarAutotyper		VAR_MAP(VT_Autotyper)
#define VarBaud                 VAR_MAP(VT_Baud)
#define VarBlinkTime		VAR_MAP(VT_BlinkTime)
#define VarCdpath		VAR_MAP(VT_Cdpath)
#define VarChild		VAR_MAP(VT_Child)
#define VarCmdHelp		VAR_MAP(VT_CmdHelp)
#define VarColorsDB		VAR_MAP(VT_ColorsDB)
#define VarCompAttachLabel	VAR_MAP(VT_CompAttachLabel)
#define VarComplete		VAR_MAP(VT_Complete)
#define VarComposeLines		VAR_MAP(VT_ComposeLines)
#define VarComposePanes		VAR_MAP(VT_ComposePanes)
#define VarCompStatusBarFmt	VAR_MAP(VT_CompStatusBarFmt)
#define VarConnected		VAR_MAP(VT_Connected)
#define VarCrt			VAR_MAP(VT_Crt)
#define VarCrtWin		VAR_REMAP(VT_CrtWin)
#define VarCwd			VAR_MAP(VT_Cwd)
#define VarDateReceived		VAR_MAP(VT_DateReceived)
#define VarDead			VAR_MAP(VT_Dead)
#define VarDeletesave		VAR_MAP(VT_Deletesave)
#define VarDetachDir		VAR_MAP(VT_DetachDir)
#define VarDisplayCharset	VAR_MAP(VT_DisplayCharset)
#define VarDisplayHeaders	VAR_MAP(VT_DisplayHeaders)
#define VarDomainRoute		VAR_MAP(VT_DomainRoute)
#define VarDot			VAR_MAP(VT_Dot)
#define VarDotLock		VAR_MAP(VT_DotLock)
#define VarEditHdrs		VAR_MAP(VT_EditHdrs)
#define VarEditor		VAR_MAP(VT_Editor)
#define VarEscape		VAR_MAP(VT_Escape)
#define VarExitSaveopts		VAR_MAP(VT_ExitSaveopts)
#define VarExpireCache		VAR_MAP(VT_ExpireCache)
#define VarExtSummaryFmt	VAR_MAP(VT_ExtSummaryFmt)
#define VarFetchTimeout		VAR_MAP(VT_FetchTimeout)
#define VarFignore		VAR_MAP(VT_Fignore)
#define VarFilec		VAR_REMAP(VT_Filec)
#define VarFileCharset		VAR_MAP(VT_FileCharset)
#define VarFilelistFmt		VAR_MAP(VT_FilelistFmt)
#define VarFileTest		VAR_MAP(VT_FileTest)
#define VarFirstPartPrimary	VAR_MAP(VT_FirstPartPrimary)
#define VarFkeyLabels		VAR_MAP(VT_FkeyLabels)
#define VarFolder		VAR_MAP(VT_Folder)
#define VarFolderTitle		VAR_MAP(VT_FolderTitle)
#define VarFolderType		VAR_MAP(VT_FolderType)
#define VarFontsDB		VAR_MAP(VT_FontsDB)
#define VarFortunates		VAR_MAP(VT_Fortunates)
#define VarFortune		VAR_MAP(VT_Fortune)
#define VarFromAddress		VAR_MAP(VT_FromAddress)
#define VarFullscreenHelp	VAR_MAP(VT_FullscreenHelp)
#define VarFunctionHelp		VAR_MAP(VT_FunctionHelp)
#define VarGuiHelp		VAR_MAP(VT_GuiHelp)
#define VarHangup		VAR_MAP(VT_Hangup)
#define VarHeaderEncoding	VAR_MAP(VT_HeaderEncoding)
#define VarHdrFormat		VAR_REMAP(VT_HdrFormat)
#define VarHidden		VAR_MAP(VT_Hidden)
#define VarHistory		VAR_MAP(VT_History)
#define VarHold			VAR_MAP(VT_Hold)
#define VarHome			VAR_MAP(VT_Home)
#define VarHostname		VAR_MAP(VT_Hostname)
#define VarIgnoreBang		VAR_MAP(VT_IgnoreBang)
#define VarIgnoreeof		VAR_MAP(VT_Ignoreeof)
#if defined( IMAP )
#define VarImapTimeout          VAR_MAP(VT_ImapTimeout)
#define VarImapUser             VAR_MAP(VT_ImapUser)
#define VarImapShared      	VAR_MAP(VT_ImapShared)
#define VarImapSynchronize      VAR_MAP(VT_ImapSynchronize)
#define VarImapCache            VAR_MAP(VT_ImapCache)
#define VarImapNoPrompt         VAR_MAP(VT_ImapNoPrompt)
#endif
#define VarInReplyTo		VAR_MAP(VT_InReplyTo)
#define VarIncomingCharset	VAR_MAP(VT_IncomingCharset)
#define VarIndentStr		VAR_MAP(VT_IndentStr)
#define VarIndexDir		VAR_MAP(VT_IndexDir)
#define VarIndexSize		VAR_MAP(VT_IndexSize)
#define VarIntrLevel		VAR_MAP(VT_IntrLevel)
#define VarIspeller		VAR_MAP(VT_Ispeller)
#define VarKeepsave		VAR_MAP(VT_Keepsave)
#define VarKnownHosts		VAR_MAP(VT_KnownHosts)
#define VarLayoutDB		VAR_MAP(VT_LayoutDB)
#define VarLdapService		VAR_MAP(VT_LdapService)
#define VarLogfile		VAR_MAP(VT_Logfile)
#define VarLookupCharset	VAR_MAP(VT_LookupCharset)
#define VarLookupFile		VAR_MAP(VT_LookupFile)
#define VarLookupHostIp		VAR_MAP(VT_LookupHostIp)
#define VarLookupMax		VAR_MAP(VT_LookupMax)
#define VarLookupMode		VAR_MAP(VT_LookupMode)
#define VarLookupSep		VAR_MAP(VT_LookupSep)
#ifdef LOOKUP_STUFF
#define VarLookupService	VAR_MAP(VT_LookupService)
#else /* !LOOKUP_STUFF */
#define VarLookupService	VAR_MAP(VT_AddressBook)
#endif /* !LOOKUP_STUFF */
#define VarMailhost		VAR_MAP(VT_Mailhost)
#define VarMailIcon		VAR_MAP(VT_MailIcon)
#define VarMailQueue		VAR_MAP(VT_MailQueue)
#define VarMailboxName		VAR_MAP(VT_MailboxName)
#define VarMainFolderTitle	VAR_MAP(VT_MainFolderTitle)
#define VarMainPanes		VAR_MAP(VT_MainPanes)
#define VarMainStatusBarFmt	VAR_MAP(VT_MainStatusBarFmt)
#define VarMaxTextLength	VAR_MAP(VT_MaxTextLength)
#define VarMbox			VAR_MAP(VT_Mbox)
#define VarMessageField		VAR_MAP(VT_MessageField)
#define VarMessageLines		VAR_MAP(VT_MessageLines)
#define VarMessagePanes		VAR_MAP(VT_MessagePanes)
#define VarMetoo		VAR_MAP(VT_Metoo)
#define VarMilTime		VAR_MAP(VT_MilTime)
#define VarMsgAttachLabel	VAR_MAP(VT_MsgAttachLabel)
#define VarMsgSeparator		VAR_MAP(VT_MsgSeparator)
#define VarMsgStatusBarFmt	VAR_MAP(VT_MsgStatusBarFmt)
#define VarMsgWin		VAR_REMAP(VT_MsgWin)
#define VarMsgWinHdrFmt		VAR_MAP(VT_MsgWinHdrFmt)
#define VarName			VAR_MAP(VT_Name)
#define VarNewline		VAR_MAP(VT_Newline)
#define VarNewmailIcon		VAR_MAP(VT_NewmailIcon)
#define VarNewmailScroll	VAR_MAP(VT_NewmailScroll)
#define VarNoExpand		VAR_MAP(VT_NoExpand)
#define VarNoHdrs		VAR_MAP(VT_NoHdrs)
#define VarNoReverse		VAR_MAP(VT_NoReverse)
#define VarNonobang		VAR_MAP(VT_Nonobang)
#define VarNosave		VAR_MAP(VT_Nosave)
#define VarPager		VAR_MAP(VT_Pager)
#define VarOrganization		VAR_MAP(VT_Organization)
#define VarOutgoingCharset	VAR_MAP(VT_OutgoingCharset)
#define VarOutput		VAR_MAP(VT_Output)
#define VarPickyMta		VAR_MAP(VT_PickyMta)
#define VarPopOptions		VAR_MAP(VT_PopOptions)
#define VarPopTimeout		VAR_MAP(VT_PopTimeout)
#define VarPopUser		VAR_MAP(VT_PopUser)
#define VarPostIndentStr	VAR_MAP(VT_PostIndentStr)
#define VarPreIndentStr		VAR_MAP(VT_PreIndentStr)
#define VarPresign		VAR_MAP(VT_Presign)
#define VarPrintCmd		VAR_MAP(VT_PrintCmd)
#define VarPrinter		VAR_MAP(VT_Printer)
#define VarPrinterCharset	VAR_MAP(VT_PrinterCharset)
#define VarPrinterOpt		VAR_MAP(VT_PrinterOpt)
#define VarPrompt		VAR_MAP(VT_Prompt)
#define VarQuiet		VAR_MAP(VT_Quiet)
#define VarRealname		VAR_MAP(VT_Realname)
#define VarRecord		VAR_MAP(VT_Record)
#define VarRecordControl	VAR_MAP(VT_RecordControl)
#define VarRecordMax		VAR_MAP(VT_RecordMax)
#define VarRecordUsers		VAR_MAP(VT_RecordUsers)
#define VarRecursive		VAR_MAP(VT_Recursive)
#define VarReplyToHdr		VAR_MAP(VT_ReplyToHdr)
#define VarSaveEmpty		VAR_MAP(VT_SaveEmpty)
#define VarScreen		VAR_MAP(VT_Screen)
#define VarScreenWin		VAR_REMAP(VT_ScreenWin)
#define VarScrollpct		VAR_MAP(VT_Scrollpct)
#define VarSendmail		VAR_MAP(VT_Sendmail)
#define VarShell		VAR_MAP(VT_Shell)
#define VarShowDeleted		VAR_MAP(VT_ShowDeleted)
#define VarSmtphost		VAR_MAP(VT_Smtphost)
#define VarSort			VAR_MAP(VT_Sort)
#ifdef PARTIAL_SEND
#define VarSplitLimit		VAR_MAP(VT_SplitLimit)
#define VarSplitSendmail	VAR_MAP(VT_SplitSendmail)
#define VarSplitSize		VAR_MAP(VT_SplitSize)
#endif /* PARTIAL_SEND */
#define VarSpeller		VAR_MAP(VT_Speller)
#define VarSqueeze		VAR_MAP(VT_Squeeze)
#define VarStatus		VAR_MAP(VT_Status)
#define VarStatusBarFmt		VAR_MAP(VT_StatusBarFmt)
#define VarSummaryFmt		VAR_MAP(VT_SummaryFmt)
#define VarSummaryLines		VAR_MAP(VT_SummaryLines)
#define VarTemplates		VAR_MAP(VT_Templates)
#define VarTextpartCharset	VAR_MAP(VT_TextpartCharset)
#define VarTimeout		VAR_MAP(VT_Timeout)
#define VarTitle		VAR_REMAP(VT_Title)
#define VarThisfolder		VAR_MAP(VT_Thisfolder)
#define VarTmpdir		VAR_MAP(VT_Tmpdir)
#define VarTombfile		VAR_REMAP(VT_Tombfile)
#define VarToolHelp		VAR_REMAP(VT_ToolHelp)
#define VarToplines		VAR_MAP(VT_Toplines)
#define VarTrustedFunctions	VAR_MAP(VT_TrustedFunctions)
#define VarUnix			VAR_MAP(VT_Unix)
#define VarUseContentLength	VAR_MAP(VT_UseContentLength)
#if defined( IMAP )
#define VarUseImap              VAR_MAP(VT_UseImap)
#endif
#define VarUseLdap		VAR_MAP(VT_UseLdap)
#define VarUsePop		VAR_MAP(VT_UsePop)
#define VarUser			VAR_MAP(VT_User)
#define VarUucpRoot		VAR_MAP(VT_UucpRoot)
#define VarVerbose		VAR_MAP(VT_Verbose)
#define VarVerify		VAR_MAP(VT_Verify)
#define VarVisual		VAR_MAP(VT_Visual)
#define VarWarning		VAR_MAP(VT_Warning)
#define VarWindowShell		VAR_MAP(VT_WindowShell)
#define VarWineditor		VAR_MAP(VT_Wineditor)
#define VarWinterm		VAR_MAP(VT_Winterm)
#define VarWrap			VAR_MAP(VT_Wrap)
#define VarWrapcolumn		VAR_MAP(VT_Wrapcolumn)
#define VarZynchost		VAR_MAP(VT_Zynchost)
#define VarZyncOptions		VAR_MAP(VT_ZyncOptions)
#define VarZyncUser		VAR_MAP(VT_ZyncUser)

#endif /* _ZM_VARS_H_ */
