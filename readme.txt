Very incomplete tools for datamining Fairy Fencer F Advent Dark Force. Partially compatible with some other compile heart games.

Use ism2.cpp on col_ground.ism2 to get a wavefront format .obj model of the floor collision of that map as well as entities like monsters, graphical effect configarations, and treasure locations.
The actual metadata attached to those objects is not dumped, just their positions and names.

Use gbin.cpp on dbDungeon.gbin, dbTreasure.gbin, and dbItems.gbin to get the information needed to know what treasures a map has. The output is tsv. Columns AD through AH of the dbDungeon output are IDs into dbTreasure, which lists a number of entries into dbItems.

DLC complicates things. You need to extract all of the dbTreasure and dbItem files you can find and search all of them.

The DB files are usually packed away inside of cl3 files. You can use the scripts linked in https://steamcommunity.com/sharedfiles/filedetails/?id=964851899 to dump cl3 and bra files. Good luck.
