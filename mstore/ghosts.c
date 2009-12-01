/* ghosts.c	Copyright 1995 Z-Code Software, a Divison of NCD */
/* 
 * $RCSfile: ghosts.c,v $
 * $Revision: 1.16 $
 * $Date: 1995/07/18 06:06:14 $
 * $Author: bobg $
 */

#ifdef ZPOP_SYNC

#include <mfolder.h>
#include <ghosts.h>
#include <except.h>
#include <excfns.h>
#include <stdio.h>
#include <hashtab.h>
#include <bfuncs.h>

static const char ghosts_rcsid[] =
    "$Id: ghosts.c,v 1.16 1995/07/18 06:06:14 bobg Exp $";

/* A ghost message is identified by its key hash.  We also record the time
 * at which it was buried, so that we can expire records in the tombfile.
 */
struct ghost {
    struct mailhash keyhash;	/* This must come first, see below */
    time_t death;
};

/* Define an exception for attempts at concurrent tomb access */
DEFINE_EXCEPTION(ghost_err_Unsealed, "ghost_err_Unsealed");

/* We presently have only one tombfile at a time.  Remember the name. */
static char tombfile[MAXPATHLEN + 1];

/* The tombfile can be open for either reading or writing; tombFILE is
 * the write pointer and channel is the read pointer.  Currently we
 * require tombFILE to be closed before we will read from channel; but
 * it's possible to open tombFILE for append, write to it, and close it
 * again, leaving channel open the whole time.  The only restriction is
 * that tombFILE must close before the next read on channel.
 *
 * Presently, reading to EOF on channel causes us to close and later reopen
 * it, because I don't know if it's safe on DOS/Windows/Mac to leave the file
 * open for reading while we're writing it.  Theoretically, though, we could
 * just pick up where we left as long as only an append had occurred.
 */
static FILE *tombFILE = 0, *channel = 0;

/* Anytime we write to tombFILE, we flag that there are fresh_graves in
 * the tomb, and indicate that the tomb is not empty.  When we read to
 * end-of-file on channel, we indicate that the tomb is empty so that
 * we know we needn't re-open it.  (The tombfile is not necessarily an
 * empty file, but we've read all that's interesting about it; it may
 * become non-empty again later if somebody appends to it.)
 *
 * The tombfile is initially treated as empty, until we know its name.
 */
static char fresh_graves = 0, tomb_empty = 1;

/* Open the tombfile for API-client operations such as burying ghosts and
 * exorcising them.  Typically this means opening for write, but the open
 * operation is actually handled lazily elsewhere, so it could be for read
 * as well.  All this does is record the tombfile name we ought to use.
 *
 * Raises the ghost_err_Unsealed exception if the tombfile name changes
 * while tombFILE is still open; see ghost_SealTomb().
 */
void
ghost_OpenTomb(tombname)
    const char *tombname;
{
    if (tombFILE)
	RAISE(ghost_err_Unsealed, "ghost_OpenTomb");
    if (tombname && *tombname) {
	if (*tombfile == 0)
	    tomb_empty = 0;	/* Our first chance at it */
	strcpy(tombfile, tombname);
    }
}

/* Close the tombFILE and make it ready for re-opening.
 *
 * May raise any of the usual efclose() errno exceptions.
 */
void
ghost_SealTomb()
{
    if (tombFILE)
	efclose(tombFILE, "ghost_SealTomb");
    tombFILE = 0;
}

/* Open the tombfile for reading via channel, provided that channel is not
 * already open (which is OK, we just don't re-open it).
 *
 * Returns nonzero if the channel is open and ready to read, zero otherwise.
 *
 * Raises ghost_err_Unsealed if the tombfile has not been sealed since the
 * last rites, ah, write.
 */
static int
ghost_AwakenDead(caller)
    const char *caller;
{
    if (fresh_graves) {
	if (tombFILE)
	    RAISE(ghost_err_Unsealed, (VPTR) caller);
	if (tomb_empty) {
	    if (channel)
		efclose(channel, (VPTR) caller);
	    channel = 0;
	    tomb_empty = 0;
	} else if (channel)
	    clearerr(channel);	/* We're not at EOF any more */
	fresh_graves = 0;
    }

    if (tomb_empty) {
	/* Should check whether some other process entombed ghosts */
	return 0;
    }

    if (!channel) {
	if (!(channel = fopen(tombfile, "r")))
	    return 0;
    }

    return 1;
}

/* Close the read channel and treat the tombfile as empty.  This should only
 * be called when we really have read all the way to end-of-file on the tomb.
 *
 * May raise errno exceptions, but it's not very likely.
 */
static void
ghost_RestInPeace(caller)
    const char *caller;
{
    if (channel)
	efclose(channel, (VPTR) caller);
    channel = 0;
    tomb_empty = 1;
}

/* Write a ghost record to the indicated file.
 *
 * May raise errno exceptions as encountered by efprintf().
 *
 * I almost named this `ghost_LastWrites', but ...
 */
static void
ghostwriter(fp, undead)
    FILE *fp;
    const struct ghost *undead;
{
    efprintf(fp, "%lu %s\n", (unsigned long)(undead->death),
	     mailhash_to_string(0, &(undead->keyhash)));
}

/* Read a ghost record from the indicated file.
 *
 * Returns zero if it fails to read a record (reaches end-of-file),
 * otherwise nonzero.
 *
 * Raises no exceptions at this time, but it probably should raise one
 * for bad record or file formats.
 */
static int
ghostwalker(fp, undead)
    FILE *fp;
    struct ghost *undead;
{
    char buf[3*MAILHASH_BYTES + 32], *next = buf;
    unsigned long val;

    if (fgets(buf, 3*MAILHASH_BYTES + 32, fp)) {
	if (sscanf(buf, "%lu", &val) != 1)
	    return 0;	/* RAISE? */
	undead->death = (time_t)val;
	while (*next && *next++ != ' ')
	    ;
	string_to_mailhash(&(undead->keyhash), next);
	return 1;
    }
    return 0;
}

/* Hashing and comparison routines for use with the hashtab dynadt.
 * There's already a perfectly good hash in the structure, so use that.
 * Lookup is by keyhash only, we don't care about time of death.
 */

static unsigned int ghost_Hash P((struct ghost *));
static int ghost_Compare P((struct ghost *, struct ghost *));

static unsigned int
ghost_Hash(spirit)
    struct ghost *spirit;
{
    unsigned int result;

    bcopy(spirit->keyhash.x, &result, sizeof(unsigned int));
    return result;
}

static int
ghost_Compare(spirit1, spirit2)
    struct ghost *spirit1, *spirit2;
{
    return mailhash_cmp(&spirit1->keyhash, &spirit2->keyhash);
}

/**/

/* Keep track of whether we've initialized the hash table. */
static char seance_in_progress;

/* Keep track in a hash table of ghosts we've contacted. */
static struct hashtab seance;

static void
seance_Init()
{
    if (! seance_in_progress) {
	hashtab_Init(&seance,
		     (unsigned int (*) P((CVPTR))) ghost_Hash,
		     (int (*) P((CVPTR, CVPTR))) ghost_Compare,
		     sizeof(struct ghost), 8);
	seance_in_progress = 1;
    }
}    

/* Commune with (that is, look up) a specific ghost in the seance by
 * probing for a matching mailhash.  If the probe fails, this mailhash
 * may not represent a ghost (but we may have to look in the tomb).
 *
 * Returns 0 if the ghost can't be contacted, 1 if it can.
 */
static int
ghost_Commune(hash)
    struct mailhash *hash;
{
    if (! seance_in_progress)
	return 0;
     
    /* The first field of a ghost is a mailhash, so we can treat
     * the input mailhash pointer as a ghost pointer for this test.
     */
    return !!hashtab_Find(&seance, (struct ghost *)hash);
}

/* Summon a ghost found in the tomb to our seance, so we can commune with
 * it later.  If a ghost has been raised and then reburied since the last
 * time we communed with it, track the most recent time of interment.
 *
 * May raise the ENOMEM exception via the hashtab dynadt.
 */
static void
ghost_Summon(spirit)
    struct ghost *spirit;
{
    struct ghost *undead;

    seance_Init();
    if (undead = (struct ghost *)hashtab_Find(&seance, spirit)) {
	/* Ghosts can be resurrected -- track the most recent demise. */
	if (undead->death < spirit->death)
	    undead->death = spirit->death;
    } else
	hashtab_Add(&seance, spirit);
}

/* Turn the indicated message into a ghost, recording its time of interment.
 *
 * Raises exceptions as per efopen(), ghostwriter(), and ghost_Summon().
 */
void
ghost_Bury(keyhash, interment)
    struct mailhash *keyhash;
    time_t interment;
{
    struct ghost undead;

    if (!tombFILE) {
	tombFILE = efopen(tombfile, "a", "ghost_Bury");
	fresh_graves = 1;
	tomb_empty = 0;		/* If channel is open, it's not at EOF */
    }
    undead.death = interment;
    bcopy(keyhash, &(undead.keyhash), (sizeof (struct mailhash)));
    ghostwriter(tombFILE, &undead);
    ghost_Summon(&undead);
}

/* Given a key hash, determine whether it refers to a ghost message.  This
 * both communes with ghosts in the seance (hash table) and awakens ghosts
 * from the tombfile in its search for the message in question.
 *
 * May raise exceptions as per ghost_AwakenDead(), ghost_Summon(), and
 * ghost_RestInPeace().
 */
int
ghostp(keyhash)
    struct mailhash *keyhash;
{
    struct ghost undead;

    if (ghost_Commune(keyhash)) {
	return 1;	/* We've already summoned this ghost */
    }
    if (ghost_AwakenDead("ghostp")) {
	while (ghostwalker(channel, &undead)) {
	    ghost_Summon(&undead);

	    /* By returning here, we avoid reading the entire file,
	     * but we leave the channel open so that we can pick up
	     * where we left off (unless new ghosts get buried).
	     */
	    if (!mailhash_cmp(keyhash, &undead.keyhash))
		return 1;
	}
	ghost_RestInPeace("ghostp");
    }

    return 0;
}

/* Summon all the ghosts from the tombfile and then truncate it, presumably
 * because we are about to rewrite it.
 *
 * Raises exceptions as per ghost_AwakenDead(), ghostwalker(), ghost_Summon(),
 * ghost_RestInPeace(), and efopen().
 */
static void
ghost_EmptyTomb()
{
    struct ghost undead;

    if (ghost_AwakenDead("ghost_EmptyTomb")) {
	while (ghostwalker(channel, &undead))
	    ghost_Summon(&undead);
	ghost_RestInPeace("ghost_EmptyTomb");
    }
    tombFILE = efopen(tombfile, "w", "ghost_EmptyTomb");
    tomb_empty = 1;
}

/* Empty the tombfile and write back to it only those ghosts that were
 * interred within the specified afterlife time.  The rest of the ghosts
 * are freed from this earthly coil and sent to whatever heaven or hell
 * awaits them.  (They might still get resurrected if a sufficiently old
 * backup tape is restored on the zpop server ....)
 *
 * Raises exceptions as per ghost_EmptyTomb() and ghostwriter().
 */
void
ghost_Exorcise(afterlife)
    const time_t afterlife;
{
    struct hashtab_iterator i;
    struct ghost *spirit;
    time_t haunting = time((time_t *)0) - afterlife;

    ghost_EmptyTomb();

    if (seance_in_progress) {
	hashtab_InitIterator(&i);
	while (spirit = (struct ghost *)hashtab_Iterate(&seance, &i)) {
	    if (haunting < spirit->death) {
		ghostwriter(tombFILE, spirit);
		tomb_empty = 0;
	    }
	}
    }
}

#endif /* ZPOP_SYNC */
