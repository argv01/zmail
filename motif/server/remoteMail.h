/*
 * Copyright 1995, Network Computing Devices, Inc.
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Network Computing
 * Devices, Inc.; the contents of this file may not be disclosed to
 * third parties, copied or duplicated in any form, in whole or in
 * part, without the prior written permission of Network Computing
 * Devices, Inc.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL NETWORK COMPUTING DEVICES,
 * INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/

#pragma once
#include <X11/Xlib.h>

#ifdef __cplusplus
extern "C" {
#define __optional(default) = (default)
#else
#define __optional(default)
#endif /* C++ */

Status ZmailMissedCall(Screen *, const char *username,
		       const char *recipient,
		       const char *subject,
		       const char *content,
		       const char *inpersonReplyName __optional(0));

Status ZmailSendFile(Screen *, const char *username,
		     const char *filename,
		     const char *recipient __optional(0),
		     const char *subject __optional(0),
		     const char *content __optional(0));

#ifdef __cplusplus
}
#undef __optional
#endif /* C++ */
