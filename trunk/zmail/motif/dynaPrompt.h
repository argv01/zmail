#ifndef INCLUDE_MOTIF_DYNAPROMPT_H
#define INCLUDE_MOTIF_DYNAPROMPT_H


#include <general.h>


struct Compose;
struct DynaPrompt;
struct FrameDataRec;


struct DynaPrompt *DynaPromptCreate P((Widget, struct Compose *));
void DynaPromptDestroy P((struct DynaPrompt *));

void DynaPromptUse P((struct DynaPrompt *, struct FrameDataRec *));
void DynaPromptRefresh P((struct DynaPrompt *));


#endif /* !INCLUDE_MOTIF_DYNAPROMPT_H */
