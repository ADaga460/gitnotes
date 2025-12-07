#ifndef SYNC_H
#define SYNC_H

int sync_to_tracked(void);

int sync_from_tracked(void);

int install_sync_hooks(void);

int reset_gitnote(int tracked_only);

#endif