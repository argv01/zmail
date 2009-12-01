#include "osconfig.h"
#include "attach.h"
#include "zfolder.h"
#include "linklist.h"
#include "prune.h"
#include "zmflag.h"


unsigned long attach_prune_size;


int
prune_omit_set(attachments, threshold)
    struct Attach * const attachments;
    const unsigned long threshold;
{
    int modifying = 0;
    
    Attach *part = is_multipart(attachments) ?
	link_next(Attach, a_link, attachments)
	: attachments;
		
    do
	if (ison(part->a_flags, AT_DELETE))
	    modifying = 1;
	else if (threshold && part->content_length > threshold) {
	    turnon(part->a_flags, AT_OMIT);
	    modifying = 1;
	}
    while ((part = link_next(Attach, a_link, part)) != attachments);

    return modifying;
}


void
prune_omit_clear(attachments)
    struct Attach * const attachments;
{
    Attach *part = is_multipart(attachments) ?
	link_next(Attach, a_link, attachments)
	: attachments;
    
    do
	turnoff(part->a_flags, AT_OMIT);
    while ((part = link_next(Attach, a_link, part)) != attachments);
}


int
prune_part_delete(folder, message, part)
    struct mfolder * const folder;
    struct Msg * const message;
    struct Attach * const part;
{
    if (folder && message && isoff(folder->mf_flags, READ_ONLY) && part && !pruned(part)) {
	turnon(part->a_flags, AT_DELETE);
	turnon(message->m_flags, DO_UPDATE);
	turnon(folder->mf_flags, DO_UPDATE);
	ZmCallbackCallAll("", ZCBTYPE_PRUNE, ZCB_PRUNE_DELETE, part);
	return 0;
    } else
	return -1;
}


int
prune_part_undelete(folder, message, part)
    struct mfolder * const folder;
    struct Msg * const message;
    struct Attach * const part;
{
    if (folder && message && isoff(folder->mf_flags, READ_ONLY) && part && !pruned(part)) {
	turnoff(part->a_flags, AT_DELETE);
	ZmCallbackCallAll("", ZCBTYPE_PRUNE, ZCB_PRUNE_UNDELETE, part);
	return 0;
    } else
	return -1;
}


const char * const prune_externalize = "Content-Type: message/external-body; access-type=x-removed-by-recipient\n\nContent-ID: <-@->\n";

int
pruned(part)
    const struct Attach * const part;
{
    return part
	&& part->mime_type == MessageExternalBody
	&& !strcmp("x-removed-by-recipient",
		   FindParam(MimeExternalParamStr(AccessTypeParam),
			     &part->content_params));
}
