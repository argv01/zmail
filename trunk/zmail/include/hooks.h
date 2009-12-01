#ifndef INCLUDE_HOOKS_H
#define INCLUDE_HOOKS_H

#define STARTUP_HOOK		"startup_hook"
#define SHUTDOWN_HOOK		"exit_hook"
#define COMPOSE_HOOK		"compose_mode_hook"
#define FETCH_MAIL_HOOK		"fetch_mail_hook"
#define RECV_MAIL_HOOK		"receive_mail_hook"

#ifdef MOTIF
#define WM_SAVE_YOURSELF_HOOK	"wm_save_yourself_hook"
#endif /* MOTIF */

#endif /* !INCLUDE_HOOKS_H */
