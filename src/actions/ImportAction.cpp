#include "Action.h"

extern "C" {
#include "global.h"
#include "action.h"
#include "create.h"
#include "data.h"
#include "error.h"
#include "find.h"
#include "hid.h"
#include "misc.h"
#include "mymem.h"
#include "pcb-printf.h"
#include "rats.h"
}

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

namespace pcb {
namespace actions {

class ImportAction : public Action {
public:
    ImportAction() : Action("Import",
        "Import schematics",
        "Import()\n"
        "Import([gnetlist|make[,source,source,...]])\n"
        "Import(setnewpoint[,(mark|center|X,Y)])\n"
        "Import(setdisperse,D,units)\n") {}

    int execute(int argc, char** argv, Coord x, Coord y) override {
        char *mode;
        char **sources = nullptr;
        int nsources = 0;

#ifdef DEBUG
        printf("ActionImport:  ===========  Entering ActionImport  ============\n");
#endif

        mode = argc > 0 ? argv[0] : nullptr;

        // Handle setdisperse sub-command
        if (mode && strcasecmp(mode, "setdisperse") == 0) {
            char *ds, *units;
            char buf[50];

            ds = argc > 1 ? argv[1] : nullptr;
            units = argc > 2 ? argv[2] : nullptr;

            if (!ds) {
                const char *as = AttributeGet(PCB, "import::disperse");
                ds = gui->prompt_for(_("Enter dispersion:"), as ? as : "0");
            }

            if (units) {
                sprintf(buf, "%s%s", ds, units);
                AttributePut(PCB, "import::disperse", buf);
            } else {
                AttributePut(PCB, "import::disperse", ds);
            }

            if (argc <= 1) {
                free(ds);
            }
            return 0;
        }

        // Handle setnewpoint sub-command
        if (mode && strcasecmp(mode, "setnewpoint") == 0) {
            const char *xs, *ys, *units;
            Coord newx, newy;
            char buf[50];

            xs = argc > 1 ? argv[1] : nullptr;
            ys = argc > 2 ? argv[2] : nullptr;
            units = argc > 3 ? argv[3] : nullptr;

            if (!xs) {
                gui->get_coords(_("Click on a location"), &newx, &newy);
            } else if (strcasecmp(xs, "center") == 0) {
                AttributeRemove(PCB, "import::newX");
                AttributeRemove(PCB, "import::newY");
                return 0;
            } else if (strcasecmp(xs, "mark") == 0) {
                if (!Marked.status) {
                    return 0;
                }
                newx = Marked.X;
                newy = Marked.Y;
            } else if (ys) {
                newx = GetValue(xs, units, nullptr);
                newy = GetValue(ys, units, nullptr);
            } else {
                Message(_("Bad syntax for Import(setnewpoint)"));
                return 1;
            }

            pcb_snprintf(buf, sizeof(buf), "%$ms", newx);
            AttributePut(PCB, "import::newX", buf);
            pcb_snprintf(buf, sizeof(buf), "%$ms", newy);
            AttributePut(PCB, "import::newY", buf);
            return 0;
        }

        // Determine import mode (gnetlist or make)
        if (!mode) {
            mode = AttributeGet(PCB, "import::mode");
        }
        if (!mode) {
            mode = (char*)"gnetlist";
        }

        // Get source files
        if (argc > 1) {
            sources = argv + 1;
            nsources = argc - 1;
        }

        if (!sources) {
            char sname[40];
            char *src;

            nsources = -1;
            do {
                nsources++;
                sprintf(sname, "import::src%d", nsources);
                src = AttributeGet(PCB, sname);
            } while (src);

            if (nsources > 0) {
                sources = (char **) malloc((nsources + 1) * sizeof(char *));
                nsources = -1;
                do {
                    nsources++;
                    sprintf(sname, "import::src%d", nsources);
                    src = AttributeGet(PCB, sname);
                    sources[nsources] = src;
                } while (src);
            }
        }

        if (!sources) {
            // Replace .pcb with .sch and hope for the best
            char *pcbname = PCB->Filename;
            char *schname;
            char *dot, *slash, *bslash;

            if (!pcbname) {
                return hid_action("ImportGUI");
            }

            schname = (char *) malloc(strlen(pcbname) + 5);
            strcpy(schname, pcbname);
            dot = strchr(schname, '.');
            slash = strchr(schname, '/');
            bslash = strchr(schname, '\\');

            if (dot && slash && dot < slash) {
                dot = nullptr;
            }
            if (dot && bslash && dot < bslash) {
                dot = nullptr;
            }
            if (dot) {
                *dot = 0;
            }
            strcat(schname, ".sch");

            if (access(schname, F_OK)) {
                free(schname);
                return hid_action("ImportGUI");
            }

            sources = (char **) malloc(2 * sizeof(char *));
            sources[0] = schname;
            sources[1] = nullptr;
            nsources = 1;
        }

        // Handle gnetlist mode
        if (strcasecmp(mode, "gnetlist") == 0) {
            char *tmpfile = tempfile_name_new("gnetlist_output");
            char **cmd;
            int i;

            if (tmpfile == nullptr) {
                Message(_("Could not create temp file"));
                return 1;
            }

            cmd = (char **) malloc((7 + nsources) * sizeof(char *));
            cmd[0] = Settings.GnetlistProgram;
            cmd[1] = (char*)"-g";
            cmd[2] = (char*)"pcbfwd";
            cmd[3] = (char*)"-o";
            cmd[4] = tmpfile;
            cmd[5] = (char*)"--";
            for (i = 0; i < nsources; i++) {
                cmd[6 + i] = sources[i];
            }
            cmd[6 + nsources] = nullptr;

#ifdef DEBUG
            printf("ActionImport:  ===========  About to run gnetlist  ============\n");
            printf("%s %s %s %s %s %s %s ...\n",
                   cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6]);
#endif

            if (pcb_spawnvp(cmd)) {
                unlink(tmpfile);
                return 1;
            }

#ifdef DEBUG
            printf("ActionImport:  ===========  About to run ActionExecuteFile, file = %s  ============\n", tmpfile);
#endif

            cmd[0] = tmpfile;
            cmd[1] = nullptr;
            ActionExecuteFile(1, cmd, 0, 0);

            free(cmd);
            tempfile_unlink(tmpfile);
        }
        // Handle make mode
        else if (strcasecmp(mode, "make") == 0) {
            int must_free_tmpfile = 0;
            char *tmpfile;
            char *cmd[10];
            int i;
            char *srclist;
            int srclen;
            char *user_outfile = nullptr;
            char *user_makefile = nullptr;
            char *user_target = nullptr;

            user_outfile = AttributeGet(PCB, "import::outfile");
            user_makefile = AttributeGet(PCB, "import::makefile");
            user_target = AttributeGet(PCB, "import::target");

            if (user_outfile && !user_target) {
                user_target = user_outfile;
            }

            if (user_outfile) {
                tmpfile = user_outfile;
            } else {
                tmpfile = tempfile_name_new("gnetlist_output");
                if (tmpfile == nullptr) {
                    Message(_("Could not create temp file"));
                    free(sources);
                    return 1;
                }
                must_free_tmpfile = 1;
            }

            srclen = sizeof("SRCLIST=") + 2;
            for (i = 0; i < nsources; i++) {
                srclen += strlen(sources[i]) + 2;
            }
            srclist = (char *) malloc(srclen);
            strcpy(srclist, "SRCLIST=");
            for (i = 0; i < nsources; i++) {
                if (i) {
                    strcat(srclist, " ");
                }
                strcat(srclist, sources[i]);
            }

            cmd[0] = Settings.MakeProgram;
            cmd[1] = (char*)"-s";
            cmd[2] = Concat("PCB=", PCB->Filename, nullptr);
            cmd[3] = srclist;
            cmd[4] = Concat("OUT=", tmpfile, nullptr);
            i = 5;
            if (user_makefile) {
                cmd[i++] = (char*)"-f";
                cmd[i++] = user_makefile;
            }
            cmd[i++] = user_target ? user_target : (char*)"pcb_import";
            cmd[i++] = nullptr;

            if (pcb_spawnvp(cmd)) {
                if (must_free_tmpfile) {
                    unlink(tmpfile);
                }
                free(cmd[2]);
                free(cmd[3]);
                free(cmd[4]);
                return 1;
            }

            cmd[0] = tmpfile;
            cmd[1] = nullptr;
            ActionExecuteFile(1, cmd, 0, 0);

            free(cmd[2]);
            free(cmd[3]);
            free(cmd[4]);
            if (must_free_tmpfile) {
                tempfile_unlink(tmpfile);
            }
        } else {
            Message(_("Unknown import mode: %s\n"), mode);
            return 1;
        }

        DeleteRats(false);
        AddAllRats(false, nullptr);

#ifdef DEBUG
        printf("ActionImport:  ===========  Leaving ActionImport  ============\n");
#endif

        return 0;
    }
};

REGISTER_ACTION(ImportAction);

}} // namespace pcb::actions
