//-----------------------------------------------------------------------------
// The text-based browser window, used to view the structure of the model by
// groups and for other similar purposes.
//
// Copyright 2008-2013 Jonathan Westhues.
//-----------------------------------------------------------------------------
#include "solvespace.h"

namespace SolveSpace {

//-----------------------------------------------------------------------------
// A navigation bar that always appears at the top of the window, with a
// link to bring us back home.
//-----------------------------------------------------------------------------
void TextWindow::ScreenHome(int link, uint32_t v) {
    SS.TW.GoToScreen(Screen::LIST_OF_GROUPS);
}
void TextWindow::ShowHeader(bool withNav) {
    ClearScreen();

    const char *header;
    std::string desc;
    if(SS.GW.LockedInWorkplane()) {
        header = "dans le plan : ";
        desc = SK.GetEntity(SS.GW.ActiveWorkplane())->DescriptionString();
    } else {
        header = "dessin / contraintes en 3D";
        desc = "";
    }

    // Navigation buttons
    if(withNav) {
        Printf(false, " %Fl%Lh%faccueil%E   %Ft%s%E%s",
            (&TextWindow::ScreenHome), header, desc.c_str());
    } else {
        Printf(false, "        %Ft%s%E%s", header, desc.c_str());
    }

    // Leave space for the icons that are painted here.
    Printf(false, "");
    Printf(false, "");
}

//-----------------------------------------------------------------------------
// The screen that shows a list of every group in the sketch, with options
// to hide or show them, and to view them in detail. This is our home page.
//-----------------------------------------------------------------------------
void TextWindow::ScreenSelectGroup(int link, uint32_t v) {
    GraphicsWindow::MenuEdit(Command::UNSELECT_ALL);
    SS.TW.GoToScreen(Screen::GROUP_INFO);
    SS.TW.shown.group.v = v;
}
void TextWindow::ScreenToggleGroupShown(int link, uint32_t v) {
    hGroup hg = { v };
    Group *g = SK.GetGroup(hg);
    g->visible = !(g->visible);
    // If a group was just shown, then it might not have been generated
    // previously, so regenerate.
    SS.GenerateAll();
}
void TextWindow::ScreenShowGroupsSpecial(int link, uint32_t v) {
    for(hGroup hg : SK.groupOrder) {
        Group *g = SK.GetGroup(hg);
        switch(link) {
        case 's': g->visible = true; break;
        case 'c': g->visible = g->solved.dof != 0; break;
        case 'h': g->visible = false; break;
        }
    }
    SS.GW.persistentDirty = true;
}
void TextWindow::ScreenActivateGroup(int link, uint32_t v) {
    SS.GW.activeGroup.v = v;
    SK.GetGroup(SS.GW.activeGroup)->Activate();
    SS.GW.ClearSuper();
}
void TextWindow::ReportHowGroupSolved(hGroup hg) {
    SS.GW.ClearSuper();
    SS.TW.GoToScreen(Screen::GROUP_SOLVE_INFO);
    SS.TW.shown.group = hg;
    SS.ScheduleShowTW();
}
void TextWindow::ScreenHowGroupSolved(int link, uint32_t v) {
    if(SS.GW.activeGroup.v != v) {
        ScreenActivateGroup(link, v);
    }
    SS.TW.GoToScreen(Screen::GROUP_SOLVE_INFO);
    SS.TW.shown.group.v = v;
}
void TextWindow::ScreenShowConfiguration(int link, uint32_t v) {
    SS.TW.GoToScreen(Screen::CONFIGURATION);
}
void TextWindow::ScreenShowEditView(int link, uint32_t v) {
    SS.TW.GoToScreen(Screen::EDIT_VIEW);
}
void TextWindow::ScreenGoToWebsite(int link, uint32_t v) {
    Platform::OpenInBrowser("http://solvespace.com/txtlink");
}
void TextWindow::ShowListOfGroups() {
    const char *radioTrue  = " " RADIO_TRUE  " ",
               *radioFalse = " " RADIO_FALSE " ",
               *checkTrue  = " " CHECK_TRUE  " ",
               *checkFalse = " " CHECK_FALSE " ";

    Printf(true, "%Ft actif");
    Printf(false, "%Ft    vu  ddl nom-du-groupe%E");
    bool afterActive = false;
    bool backgroundParity = false;
    for(hGroup hg : SK.groupOrder) {
        Group *g = SK.GetGroup(hg);

        std::string s = g->DescriptionString();
        bool active = (g->h == SS.GW.activeGroup);
        bool shown = g->visible;
        bool ok = g->IsSolvedOkay();
        bool warn = (g->type == Group::Type::DRAWING_WORKPLANE &&
                     g->polyError.how != PolyError::GOOD) ||
                    ((g->type == Group::Type::EXTRUDE ||
                      g->type == Group::Type::LATHE) &&
                     SK.GetGroup(g->opA)->polyError.how != PolyError::GOOD);
        int dof = g->solved.dof;
        char sdof[16] = "ok ";
        if(ok && dof > 0) {
            if(dof > 999) {
              strcpy(sdof, "###");
            } else {
              sprintf(sdof, "%-3d", dof);
            }
        }
        std::string suffix;
        if(g->forceToMesh || g->IsTriangleMeshAssembly()) {
            suffix = " (∆)";
        }

        bool ref = (g->h == Group::HGROUP_REFERENCES);
        Printf(false,
               "%Bp%Fd "
               "%Ft%s%Fb%D%f%Ll%s%E "
               "%Fb%s%D%f%Ll%s%E  "
               "%Fp%D%f%s%Ll%s%E "
               "%Fp%Ll%D%f%s%E%s",
               // Alternate between light and dark backgrounds, for readability
               backgroundParity ? 'd' : 'a',
               // Link that activates the group
               ref ? "   " : "",
               g->h.v, (&TextWindow::ScreenActivateGroup),
               ref ? "" : (active ? radioTrue : radioFalse),
               // Link that hides or shows the group
               afterActive ? " - " : "",
               g->h.v, (&TextWindow::ScreenToggleGroupShown),
               afterActive ? "" : (shown ? checkTrue : checkFalse),
               // Link to the errors, if a problem occurred while solving
               ok ? (warn ? 'm' : (dof > 0 ? 'i' : 's')) : 'x',
               g->h.v, (&TextWindow::ScreenHowGroupSolved),
               ok ? ((warn && SS.checkClosedContour) ? "err" : sdof) : "",
               ok ? "" : "ERR",
               // Link to a screen that gives more details on the group
               g->suppress ? 'g' : 'l',
               g->h.v, (&TextWindow::ScreenSelectGroup), s.c_str(),
               suffix.c_str());

        if(active) afterActive = true;
        backgroundParity = !backgroundParity;
    }

    Printf(true, "  %Fl%Ls%ftout voir%E / %Fl%Lc%fnon contraints%E / %Fl%Lh%ftout cacher%E",
        &(TextWindow::ScreenShowGroupsSpecial),
        &(TextWindow::ScreenShowGroupsSpecial),
        &(TextWindow::ScreenShowGroupsSpecial));
    Printf(true,  "  %Fl%Ls%fstyles de ligne%E /"
                   " %Fl%Ls%fvue%E /"
                   " %Fl%Ls%fconfiguration%E",
        &(TextWindow::ScreenShowListOfStyles),
        &(TextWindow::ScreenShowEditView),
        &(TextWindow::ScreenShowConfiguration));
}


//-----------------------------------------------------------------------------
// The screen that shows information about a specific group, and allows the
// user to edit various things about it.
//-----------------------------------------------------------------------------
void TextWindow::ScreenHoverGroupWorkplane(int link, uint32_t v) {
    SS.GW.hover.Clear();
    hGroup hg = { v };
    SS.GW.hover.entity = hg.entity(0);
    SS.GW.hover.emphasized = true;
}
void TextWindow::ScreenHoverEntity(int link, uint32_t v) {
    SS.GW.hover.Clear();
    hEntity he = { v };
    SS.GW.hover.entity = he;
    SS.GW.hover.emphasized = true;
}
void TextWindow::ScreenHoverRequest(int link, uint32_t v) {
    SS.GW.hover.Clear();
    hRequest hr = { v };
    SS.GW.hover.entity = hr.entity(0);
    SS.GW.hover.emphasized = true;
}
void TextWindow::ScreenHoverConstraint(int link, uint32_t v) {
    if( SS.GW.showConstraints == GraphicsWindow::ShowConstraintMode::SCM_NOSHOW ) return;
    hConstraint hc = { v };
    SS.GW.hover.Clear();
    SS.GW.hover.constraint = hc;
    SS.GW.hover.emphasized = true;
}
void TextWindow::ScreenSelectEntity(int link, uint32_t v) {
    SS.GW.ClearSelection();
    GraphicsWindow::Selection sel = {};
    hEntity he = { v };
    sel.entity = he;
    SS.GW.selection.Add(&sel);
}
void TextWindow::ScreenSelectRequest(int link, uint32_t v) {
    SS.GW.ClearSelection();
    GraphicsWindow::Selection sel = {};
    hRequest hr = { v };
    sel.entity = hr.entity(0);
    SS.GW.selection.Add(&sel);
}
void TextWindow::ScreenSelectConstraint(int link, uint32_t v) {
    SS.GW.ClearSelection();
    GraphicsWindow::Selection sel = {};
    sel.constraint.v = v;
    SS.GW.selection.Add(&sel);
}

void TextWindow::ScreenChangeGroupOption(int link, uint32_t v) {
    SS.UndoRemember();
    Group *g = SK.GetGroup(SS.TW.shown.group);

    switch(link) {
        case 's':
            if(g->subtype == Group::Subtype::TWO_SIDED)
                g->subtype = Group::Subtype::ONE_SIDED;
            if(g->subtype == Group::Subtype::TWO_SKEWED)
                g->subtype = Group::Subtype::ONE_SKEWED;
            break;
        case 'S':
            if(g->subtype == Group::Subtype::ONE_SIDED)
                g->subtype = Group::Subtype::TWO_SIDED;
            if(g->subtype == Group::Subtype::ONE_SKEWED)
                g->subtype = Group::Subtype::TWO_SKEWED;
            break;
        case 'w':
            if(g->subtype == Group::Subtype::ONE_SIDED) {
                g->subtype = Group::Subtype::ONE_SKEWED;
                break;
            }
            if(g->subtype == Group::Subtype::TWO_SIDED) {
                g->subtype = Group::Subtype::TWO_SKEWED;
                break;
            }
            if(g->subtype == Group::Subtype::ONE_SKEWED) {
                g->subtype = Group::Subtype::ONE_SIDED;
                break;
            }
            if(g->subtype == Group::Subtype::TWO_SKEWED) {
                g->subtype = Group::Subtype::TWO_SIDED;
                break;
            }
            break;

        case 'k': g->skipFirst = true; break;
        case 'K': g->skipFirst = false; break;

        case 'c':
            if(g->type == Group::Type::EXTRUDE) {
                // When an extrude group is first created, it's positioned for a union
                // extrusion. If no constraints were added, flip it when we switch between
                // union/assemble and difference/intersection modes to avoid manual work doing the same.
                if(g->meshCombine != (Group::CombineAs)v && g->GetNumConstraints() == 0) {
                    // I apologise for this if statement
                    if((((Group::CombineAs::DIFFERENCE == g->meshCombine) ||
                         (Group::CombineAs::INTERSECTION == g->meshCombine)) &&
                        (Group::CombineAs::DIFFERENCE != (Group::CombineAs)v) &&
                        (Group::CombineAs::INTERSECTION != (Group::CombineAs)v)) ||
                       ((Group::CombineAs::DIFFERENCE != g->meshCombine) &&
                        (Group::CombineAs::INTERSECTION != g->meshCombine) &&
                        ((Group::CombineAs::DIFFERENCE == (Group::CombineAs)v) ||
                         (Group::CombineAs::INTERSECTION == (Group::CombineAs)v)))) {
                        g->ExtrusionForceVectorTo(g->ExtrusionGetVector().Negated());
                    }
                }
            }
            g->meshCombine = (Group::CombineAs)v;
            break;

        case 'P': g->suppress = !(g->suppress); break;

        case 'r': g->relaxConstraints = !(g->relaxConstraints); break;

        case 'e': g->allowRedundant = !(g->allowRedundant); break;

        case 'D': g->suppressDofCalculation = !(g->suppressDofCalculation); break;

        case 'v': g->visible = !(g->visible); break;

        case 'd': g->allDimsReference = !(g->allDimsReference); break;

        case 'f': g->forceToMesh = !(g->forceToMesh); break;
    }

    SS.MarkGroupDirty(g->h);
    SS.GW.ClearSuper();
}

void TextWindow::ScreenColor(int link, uint32_t v) {
    SS.UndoRemember();

    Group *g = SK.GetGroup(SS.TW.shown.group);
    SS.TW.ShowEditControlWithColorPicker(3, g->color);
    SS.TW.edit.meaning = Edit::GROUP_COLOR;
}
void TextWindow::ScreenOpacity(int link, uint32_t v) {
    Group *g = SK.GetGroup(SS.TW.shown.group);

    SS.TW.ShowEditControl(11, ssprintf("%.2f", g->color.alphaF()));
    SS.TW.edit.meaning = Edit::GROUP_OPACITY;
    SS.TW.edit.group = g->h;
}
void TextWindow::ScreenChangeExprA(int link, uint32_t v) {
    Group *g = SK.GetGroup(SS.TW.shown.group);

    SS.TW.ShowEditControl(10, ssprintf("%d", (int)g->valA));
    SS.TW.edit.meaning = Edit::TIMES_REPEATED;
    SS.TW.edit.group.v = v;
}
void TextWindow::ScreenChangeGroupName(int link, uint32_t v) {
    Group *g = SK.GetGroup(SS.TW.shown.group);
    SS.TW.ShowEditControl(12, g->DescriptionString().substr(5));
    SS.TW.edit.meaning = Edit::GROUP_NAME;
    SS.TW.edit.group.v = v;
}
void TextWindow::ScreenChangeGroupScale(int link, uint32_t v) {
    Group *g = SK.GetGroup(SS.TW.shown.group);

    SS.TW.ShowEditControl(13, ssprintf("%.3f", g->scale));
    SS.TW.edit.meaning = Edit::GROUP_SCALE;
    SS.TW.edit.group.v = v;
}
void TextWindow::ScreenChangeHelixPitch(int link, uint32_t v) {
    Group *g = SK.GetGroup(SS.TW.shown.group);
    double pitch = g->valB/SS.MmPerUnit();
    SS.TW.ShowEditControl(3, ssprintf("%.8f", pitch));
    SS.TW.edit.meaning = Edit::HELIX_PITCH;
    SS.TW.edit.group.v = v;
}
void TextWindow::ScreenChangePitchOption(int link, uint32_t v) {
    Group *g = SK.GetGroup(SS.TW.shown.group);
    if(g->valB == 0.0) {
        g->valB = SK.GetParam(g->h.param(7))->val * PI /
              (SK.GetParam(g->h.param(3))->val);
    } else {
        g->valB = 0.0;
    }
    SS.GW.Invalidate();
}
void TextWindow::ScreenDeleteGroup(int link, uint32_t v) {
    SS.UndoRemember();

    hGroup hg = SS.TW.shown.group;
    if(hg == SS.GW.activeGroup) {
        SS.GW.activeGroup = SK.GetGroup(SS.GW.activeGroup)->PreviousGroup()->h;
    }

    // Reset the text window, since we're displaying information about
    // the group that's about to get deleted.
    SS.TW.ClearSuper();

    // This is a major change, so let's re-solve everything.
    SK.group.RemoveById(hg);
    SS.GenerateAll(SolveSpaceUI::Generate::ALL);

    // Reset the graphics window. This will also recreate the default
    // group if it was removed.
    SS.GW.ClearSuper();
}
void TextWindow::ShowGroupInfo() {
    Group *g = SK.GetGroup(shown.group);
    const char *s = "???";

    if(shown.group == Group::HGROUP_REFERENCES) {
        Printf(true, "%FtGROUPE  %E%s", g->DescriptionString().c_str());
        goto list_items;
    } else {
        Printf(true, "%FtGROUPE  %E%s [%Fl%Ll%D%frenommer%E/%Fl%Ll%D%fsuppr.%E]",
            g->DescriptionString().c_str(),
            g->h.v, &TextWindow::ScreenChangeGroupName,
            g->h.v, &TextWindow::ScreenDeleteGroup);
    }

    if(g->type == Group::Type::LATHE) {
        Printf(true, " %Fttourner l'esquisse plane");
    } else if(g->type == Group::Type::EXTRUDE || g->type == Group::Type::ROTATE ||
              g->type == Group::Type::TRANSLATE || g->type == Group::Type::REVOLVE ||
              g->type == Group::Type::HELIX) {
        if(g->type == Group::Type::EXTRUDE) {
            s = "extruder l'esquisse plane";
        } else if(g->type == Group::Type::TRANSLATE) {
            s = "translater l'esquisse d'origine";
        } else if(g->type == Group::Type::HELIX) {
            s = "créer une extrusion hélicoïdale";
        } else if(g->type == Group::Type::ROTATE) {
            s = "pivoter l'esquisse d'origine";
        } else if(g->type == Group::Type::REVOLVE) {
            s = "révolution de l'esquisse d'origine";
        }
        Printf(true, " %Ft%s%E", s);

        bool one  = ((g->subtype == Group::Subtype::ONE_SIDED) ||
                     (g->subtype == Group::Subtype::ONE_SKEWED));
        bool two  = ((g->subtype == Group::Subtype::TWO_SIDED) ||
                     (g->subtype == Group::Subtype::TWO_SKEWED));
        bool skew = ((g->subtype == Group::Subtype::ONE_SKEWED) ||
                     (g->subtype == Group::Subtype::TWO_SKEWED));
        
        if (g->type == Group::Type::EXTRUDE) {
        Printf(false,
            "%Ba   %f%Ls%Fd%s un côté%E  "
                  "%f%LS%Fd%s deux côtés%E  "
                  "%f%Lw%Fd%s inclinée%E",
            &TextWindow::ScreenChangeGroupOption,
            one ? RADIO_TRUE : RADIO_FALSE,
            &TextWindow::ScreenChangeGroupOption,
            two ? RADIO_TRUE : RADIO_FALSE,
            &TextWindow::ScreenChangeGroupOption,
            skew ? CHECK_TRUE : CHECK_FALSE);
        } else {
        Printf(false,
            "%Ba   %f%Ls%Fd%s un côté%E  "
                  "%f%LS%Fd%s deux côtés%E",
            &TextWindow::ScreenChangeGroupOption,
            one ? RADIO_TRUE : RADIO_FALSE,
            &TextWindow::ScreenChangeGroupOption,
            two ? RADIO_TRUE : RADIO_FALSE);        
        }
        
        if(g->type == Group::Type::ROTATE || g->type == Group::Type::TRANSLATE) {
            if(g->subtype == Group::Subtype::ONE_SIDED) {
                bool skip = g->skipFirst;
                Printf(false,
                   "%Bd   %Ftdébut  %f%LK%Fd%s avec l'original%E  "
                         "%f%Lk%Fd%s avec la copie n°1%E",
                    &ScreenChangeGroupOption,
                    !skip ? RADIO_TRUE : RADIO_FALSE,
                    &ScreenChangeGroupOption,
                    skip ? RADIO_TRUE : RADIO_FALSE);
            }

            int times = (int)(g->valA);
            Printf(false, "%Bp   %Ftrépéter%E %d fois%s %Fl%Ll%D%f[modifier]%E",
                (g->subtype == Group::Subtype::ONE_SIDED) ? 'a' : 'd',
                times, "",
                g->h.v, &TextWindow::ScreenChangeExprA);
        }
    } else if(g->type == Group::Type::LINKED) {
        Printf(true, " %Ftgéométrie liée depuis un fichier%E");
        Platform::Path relativePath = g->linkFile.RelativeTo(SS.saveFile.Parent());
        if(relativePath.IsEmpty()) {
            Printf(false, "%Ba   '%s'", g->linkFile.raw.c_str());
        } else {
            Printf(false, "%Ba   '%s'", relativePath.raw.c_str());
        }
        Printf(false, "%Bd   %Ftéchelle%E %# %Fl%Ll%f%D[modifier]%E",
            g->scale,
            &TextWindow::ScreenChangeGroupScale, g->h.v);
    } else if(g->type == Group::Type::DRAWING_3D) {
        Printf(true, " %Ftdessin en 3D%E");
    } else if(g->type == Group::Type::DRAWING_WORKPLANE) {
        Printf(true, " %Ftdessin dans un nouveau plan de travail%E");
    } else {
        Printf(true, "???");
    }
    Printf(false, "");

    if(g->type == Group::Type::HELIX) {
        Printf(false, "%Ft pas - avance par tour%E");

        if (fabs(g->valB) != 0.0) {
            Printf(false, "  %Ba %# %Fl%Ll%f%D[modifier]%E",
            g->valB / SS.MmPerUnit(),
            &TextWindow::ScreenChangeHelixPitch, g->h.v);
        } else {
            Printf(false, "  %Ba %# %E",
              SK.GetParam(g->h.param(7))->val * PI /
              ( (SK.GetParam(g->h.param(3))->val) * SS.MmPerUnit() ),
              &TextWindow::ScreenChangeHelixPitch, g->h.v);
        }
        Printf(false, "   %Fd%f%LP%s  fixe",
            &TextWindow::ScreenChangePitchOption,
            g->valB != 0 ? CHECK_TRUE : CHECK_FALSE);

        Printf(false, ""); // blank line    
    }

    if(g->type == Group::Type::EXTRUDE || g->type == Group::Type::LATHE ||
       g->type == Group::Type::REVOLVE || g->type == Group::Type::LINKED ||
       g->type == Group::Type::HELIX) {
        bool un   = (g->meshCombine == Group::CombineAs::UNION);
        bool diff = (g->meshCombine == Group::CombineAs::DIFFERENCE);
        bool intr = (g->meshCombine == Group::CombineAs::INTERSECTION);
        bool asy  = (g->meshCombine == Group::CombineAs::ASSEMBLE);

        Printf(false, " %Ftmodèle solide en");
        Printf(false, "%Ba   %f%D%Lc%Fd%s union%E  "
                             "%f%D%Lc%Fd%s assemblage%E  ",
            &TextWindow::ScreenChangeGroupOption,
            Group::CombineAs::UNION,
            un ? RADIO_TRUE : RADIO_FALSE,
            &TextWindow::ScreenChangeGroupOption,
            Group::CombineAs::ASSEMBLE,
            (asy ? RADIO_TRUE : RADIO_FALSE));
        Printf(false, "%Ba   %f%D%Lc%Fd%s différence%E  "
                             "%f%D%Lc%Fd%s intersection%E  ",
            &TextWindow::ScreenChangeGroupOption,
            Group::CombineAs::DIFFERENCE,
            diff ? RADIO_TRUE : RADIO_FALSE,
            &TextWindow::ScreenChangeGroupOption,
            Group::CombineAs::INTERSECTION,
            intr ? RADIO_TRUE : RADIO_FALSE);

        if(g->type == Group::Type::EXTRUDE || g->type == Group::Type::LATHE ||
           g->type == Group::Type::REVOLVE || g->type == Group::Type::HELIX) {
            Printf(false,
                "%Bd   %Ftcouleur %E%Bz  %Bd (%@, %@, %@) %f%D%Lf%Fl[modifier]%E",
                &g->color,
                g->color.redF(), g->color.greenF(), g->color.blueF(),
                ScreenColor, top[rows-1] + 2);
            Printf(false, "%Bd   %Ftopacité%E %@ %f%Lf%Fl[modifier]%E",
                g->color.alphaF(),
                &TextWindow::ScreenOpacity);
        }

        if(g->type == Group::Type::EXTRUDE || g->type == Group::Type::LATHE ||
           g->type == Group::Type::REVOLVE || g->type == Group::Type::LINKED ||
           g->type == Group::Type::HELIX) {
            Printf(false, "   %Fd%f%LP%s  désactiver le volume de ce groupe",
                &TextWindow::ScreenChangeGroupOption,
                g->suppress ? CHECK_TRUE : CHECK_FALSE);
        }

        Printf(false, "");
    }

    Printf(false, " %f%Lv%Fd%s  afficher les entités de ce groupe",
        &TextWindow::ScreenChangeGroupOption,
        g->visible ? CHECK_TRUE : CHECK_FALSE);

    if(!g->IsForcedToMeshBySource() && !g->IsTriangleMeshAssembly()) {
        Printf(false, " %f%Lf%Fd%s  convertir les surfaces NURBS en maillage",
            &TextWindow::ScreenChangeGroupOption,
            g->forceToMesh ? CHECK_TRUE : CHECK_FALSE);
    } else {
        Printf(false, " (modèle déjà converti en maillage)");
    }

    Printf(true, " %f%Lr%Fd%s  assouplir contraintes et cotes",
        &TextWindow::ScreenChangeGroupOption,
        g->relaxConstraints ? CHECK_TRUE : CHECK_FALSE);

    Printf(false, " %f%Le%Fd%s  autoriser les contraintes redondantes",
        &TextWindow::ScreenChangeGroupOption,
        g->allowRedundant ? CHECK_TRUE : CHECK_FALSE);

    Printf(false, " %f%LD%Fd%s  ne pas calculer les ddl (solveur plus rapide)",
        &TextWindow::ScreenChangeGroupOption,
        g->suppressDofCalculation ? CHECK_TRUE : CHECK_FALSE);

    Printf(false, " %f%Ld%Fd%s  toutes les cotes en indicatif",
        &TextWindow::ScreenChangeGroupOption,
        g->allDimsReference ? CHECK_TRUE : CHECK_FALSE);

    if(g->booleanFailed) {
        Printf(false, "");
        Printf(false, "L'opération booléenne a échoué. Il est peut-être ");
        Printf(false, "possible de corriger le problème en cochant ");
        Printf(false, "« convertir les surfaces NURBS en maillage ».");
    }

list_items:
    Printf(false, "");
    Printf(false, "%Ft éléments du groupe");

    int a = 0;
    for(auto &r : SK.request) {

        if(r.group == shown.group) {
            std::string s = r.DescriptionString();
            Printf(false, "%Bp   %Fl%Ll%D%f%h%s%E",
                   (a & 1) ? 'd' : 'a',
                   r.h.v,
                   (&TextWindow::ScreenSelectRequest),
                   &(TextWindow::ScreenHoverRequest),
                   s.c_str());
            a++;
        }
    }
    if(a == 0) Printf(false, "%Ba   (aucun)");

    a = 0;
    Printf(false, "");
    Printf(false, "%Ft contraintes du groupe (%d ddl)", g->solved.dof);
    for(auto &c : SK.constraint) {

        if(c.group == shown.group) {
            std::string s = c.DescriptionString();
            Printf(false, "%Bp   %Fl%Ll%D%f%h%s%E %s",
                   (a & 1) ? 'd' : 'a',
                   c.h.v,
                   (&TextWindow::ScreenSelectConstraint),
                   (&TextWindow::ScreenHoverConstraint),
                   s.c_str(),
                   c.reference ? "(réf)" : "");
            a++;
        }
    }
    if(a == 0) Printf(false, "%Ba   (aucun)");
}

//-----------------------------------------------------------------------------
// The screen that's displayed when the sketch fails to solve. A report of
// what failed, and (if the problem is a singular Jacobian) a list of
// constraints that could be removed to fix it.
//-----------------------------------------------------------------------------
void TextWindow::ScreenAllowRedundant(int link, uint32_t v) {
    SS.UndoRemember();

    Group *g = SK.GetGroup(SS.TW.shown.group);
    g->allowRedundant = true;
    SS.MarkGroupDirty(SS.TW.shown.group);

    SS.TW.shown.screen = Screen::GROUP_INFO;
    SS.TW.Show();
}
void TextWindow::ShowGroupSolveInfo() {
    Group *g = SK.GetGroup(shown.group);
    if(g->IsSolvedOkay()) {
        // Go back to the default group info screen
        shown.screen = Screen::GROUP_INFO;
        Show();
        return;
    }

    Printf(true, "%FtGROUPE   %E%s", g->DescriptionString().c_str());
    switch(g->solved.how) {
        case SolveResult::DIDNT_CONVERGE:
            Printf(true, "%FxÉCHEC DU SOLVEUR !%Fd contraintes insolubles");
            Printf(true, "les contraintes suivantes sont incompatibles");
            break;

        case SolveResult::REDUNDANT_DIDNT_CONVERGE:
            Printf(true, "%FxÉCHEC DU SOLVEUR !%Fd contraintes insolubles");
            Printf(true, "les contraintes suivantes ne sont pas satisfaites");
            break;

        case SolveResult::REDUNDANT_OKAY:
            Printf(true, "%FxÉCHEC DU SOLVEUR !%Fd contraintes redondantes");
            Printf(true, "supprimez l'une d'elles pour corriger");
            break;

        case SolveResult::TOO_MANY_UNKNOWNS:
            Printf(true, "Trop d'inconnues dans un seul groupe !");
            return;

        default: ssassert(false, "Unexpected solve result");
    }

    for(int i = 0; i < g->solved.remove.n; i++) {
        hConstraint hc = g->solved.remove[i];
        Constraint *c = SK.constraint.FindByIdNoOops(hc);
        if(!c) continue;

        Printf(false, "%Bp   %Fl%Ll%D%f%h%s%E",
            (i & 1) ? 'd' : 'a',
            c->h.v, (&TextWindow::ScreenSelectConstraint),
            (&TextWindow::ScreenHoverConstraint),
            c->DescriptionString().c_str());
    }

    if(g->solved.timeout) {
        Printf(true,  "%FxCertains éléments ont été omis de la liste%Fd");
        Printf(false,  "%Fxcar l'opération a pris trop de temps.%Fd");
    }

    Printf(true,  "Le problème peut peut-être se corriger ");
    Printf(false, "avec Editer -> Annuler.");

    if(g->solved.how == SolveResult::REDUNDANT_OKAY) {
        Printf(true,  "Pour masquer cette erreur, ");
        Printf(false, "%Fl%f%Llautorisez les contraintes redondantes%E dans ",
                      &TextWindow::ScreenAllowRedundant);
        Printf(false, "ce groupe.");
    }
}

//-----------------------------------------------------------------------------
// When we're stepping a dimension. User specifies the finish value, and
// how many steps to take in between current and finish, re-solving each
// time.
//-----------------------------------------------------------------------------
void TextWindow::ScreenStepDimFinish(int link, uint32_t v) {
    SS.TW.edit.meaning = Edit::STEP_DIM_FINISH;
    std::string edit_value;
    if(SS.TW.stepDim.isDistance) {
        edit_value = SS.MmToString(SS.TW.stepDim.finish, true);
    } else {
        edit_value = ssprintf("%.3f", SS.TW.stepDim.finish);
    }
    SS.TW.ShowEditControl(12, edit_value);
}
void TextWindow::ScreenStepDimSteps(int link, uint32_t v) {
    SS.TW.edit.meaning = Edit::STEP_DIM_STEPS;
    SS.TW.ShowEditControl(12, ssprintf("%d", SS.TW.stepDim.steps));
}
void TextWindow::ScreenStepDimGo(int link, uint32_t v) {
    hConstraint hc = SS.TW.shown.constraint;
    Constraint *c = SK.constraint.FindByIdNoOops(hc);
    if(c) {
        SS.UndoRemember();

        double start = c->valA, finish = SS.TW.stepDim.finish;
        SS.TW.stepDim.time = GetMilliseconds();
        SS.TW.stepDim.step = 1;

        if(!SS.TW.stepDim.timer) {
            SS.TW.stepDim.timer = Platform::CreateTimer();
        }
        SS.TW.stepDim.timer->onTimeout = [=] {
            if(SS.TW.stepDim.step <= SS.TW.stepDim.steps) {
                c->valA = start + ((finish - start)*SS.TW.stepDim.step)/SS.TW.stepDim.steps;
                SS.MarkGroupDirty(c->group);
                SS.GenerateAll();
                if(!SS.ActiveGroupsOkay()) {
                    // Failed to solve, so quit
                    return;
                }
                SS.TW.stepDim.step++;

                const int64_t STEP_MILLIS = 50;
                int64_t time = GetMilliseconds();
                if(time - SS.TW.stepDim.time < STEP_MILLIS) {
                    SS.TW.stepDim.timer->RunAfterNextFrame();
                } else {
                    SS.TW.stepDim.timer->RunAfter((unsigned)(time - SS.TW.stepDim.time - STEP_MILLIS));
                }
                SS.TW.stepDim.time = time;
            } else {
                SS.TW.GoToScreen(Screen::LIST_OF_GROUPS);
                SS.ScheduleShowTW();
            }
            SS.GW.Invalidate();
        };
        SS.TW.stepDim.timer->RunAfterNextFrame();
    }
}
void TextWindow::ShowStepDimension() {
    Constraint *c = SK.constraint.FindByIdNoOops(shown.constraint);
    if(!c) {
        shown.screen = Screen::LIST_OF_GROUPS;
        Show();
        return;
    }

    Printf(true, "%FtCOTE PAS-À-PAS%E %s", c->DescriptionString().c_str());

    if(stepDim.isDistance) {
        Printf(true,  "%Ba   %Ftdébut%E    %s", SS.MmToString(c->valA).c_str());
        Printf(false, "%Bd   %Ftfin%E      %s %Fl%Ll%f[modifier]%E",
            SS.MmToString(stepDim.finish).c_str(), &ScreenStepDimFinish);
    } else {
        Printf(true,  "%Ba   %Ftdébut%E    %@", c->valA);
        Printf(false, "%Bd   %Ftfin%E      %@ %Fl%Ll%f[modifier]%E",
            stepDim.finish, &ScreenStepDimFinish);
    }
    Printf(false, "%Ba   %Ftétapes%E   %d %Fl%Ll%f%D[modifier]%E",
        stepDim.steps, &ScreenStepDimSteps);

    Printf(true, " %Fl%Ll%flancer le pas-à-pas%E", &ScreenStepDimGo);

    Printf(true, "(ou %Fl%Ll%fannuler l'opération%E)", &ScreenHome);
}

//-----------------------------------------------------------------------------
// When we're creating tangent arcs (as requests, not as some parametric
// thing). User gets to specify the radius, and whether the old untrimmed
// curves are kept or deleted.
//-----------------------------------------------------------------------------
void TextWindow::ScreenChangeTangentArc(int link, uint32_t v) {
    switch(link) {
        case 'r': {
            SS.TW.edit.meaning = Edit::TANGENT_ARC_RADIUS;
            SS.TW.ShowEditControl(3, SS.MmToString(SS.tangentArcRadius, true));
            break;
        }

        case 'a': SS.tangentArcManual = !SS.tangentArcManual; break;
        case 'm': SS.tangentArcModify = !SS.tangentArcModify; break;
    }
}
void TextWindow::ShowTangentArc() {
    Printf(true, "%FtPARAMÈTRES DE L'ARC TANGENT%E");

    Printf(true,  "%Ft rayon de l'arc créé%E");
    if(SS.tangentArcManual) {
        Printf(false, "%Ba   %s %Fl%Lr%f[modifier]%E",
            SS.MmToString(SS.tangentArcRadius).c_str(),
            &(TextWindow::ScreenChangeTangentArc));
    } else {
        Printf(false, "%Ba   automatique");
    }

    Printf(false, "");
    Printf(false, "  %Fd%f%La%s  rayon automatique%E",
        &ScreenChangeTangentArc,
        !SS.tangentArcManual ? CHECK_TRUE : CHECK_FALSE);
    Printf(false, "  %Fd%f%Lm%s  modifier les entités d'origine%E",
        &ScreenChangeTangentArc,
        SS.tangentArcModify ? CHECK_TRUE : CHECK_FALSE);

    Printf(false, "");
    Printf(false, "Pour créer un arc tangent en un point,");
    Printf(false, "sélectionnez ce point puis choisissez");
    Printf(false, "Dessin -> Rendre l'Arc Tangent au Point.");
    Printf(true, "(ou %Fl%Ll%fretour à l'accueil%E)", &ScreenHome);
}

//-----------------------------------------------------------------------------
// The edit control is visible, and the user just pressed enter.
//-----------------------------------------------------------------------------
void TextWindow::EditControlDone(std::string s) {
    edit.showAgain = false;

    switch(edit.meaning) {
        case Edit::TIMES_REPEATED:
            if(Expr *e = Expr::From(s, /*popUpError=*/true)) {
                SS.UndoRemember();

                double ev = e->Eval();
                if((int)ev < 1) {
                    Error(_("Can't repeat fewer than 1 time."));
                    break;
                }
                if((int)ev > 999) {
                    Error(_("Can't repeat more than 999 times."));
                    break;
                }

                Group *g = SK.GetGroup(edit.group);
                g->valA = ev;

                if(g->type == Group::Type::ROTATE) {
                    // If the group does not contain any constraints, then
                    // set the numerical guess to space the copies uniformly
                    // over one rotation. Don't touch the guess if we're
                    // already constrained, because that would break
                    // convergence.
                    if(g->GetNumConstraints() == 0) {
                        double copies = (g->skipFirst) ? (ev + 1) : ev;
                        SK.GetParam(g->h.param(3))->val = PI/(2*copies);
                    }
                }

                SS.MarkGroupDirty(g->h);
            }
            break;

        case Edit::GROUP_NAME:
            if(s.empty()) {
                Error(_("Group name cannot be empty"));
            } else {
                SS.UndoRemember();

                Group *g = SK.GetGroup(edit.group);
                g->name = s;
            }
            break;

        case Edit::GROUP_SCALE:
            if(Expr *e = Expr::From(s, /*popUpError=*/true)) {
                double ev = e->Eval();
                if(fabs(ev) < 1e-6) {
                    Error(_("Scale cannot be zero."));
                } else {
                    Group *g = SK.GetGroup(edit.group);
                    g->scale = ev;
                    SS.MarkGroupDirty(g->h);
                }
            }
            break;

        case Edit::HELIX_PITCH:  // stored in valB
            if(Expr *e = Expr::From(s, /*popUpError=*/true)) {
                double ev = e->Eval();
                Group *g = SK.GetGroup(edit.group);
                g->valB = ev * SS.MmPerUnit();
                SS.MarkGroupDirty(g->h);
            }
            break;

        case Edit::GROUP_COLOR: {
            Vector rgb;
            if(sscanf(s.c_str(), "%lf, %lf, %lf", &rgb.x, &rgb.y, &rgb.z)==3) {
                rgb = rgb.ClampWithin(0, 1);

                Group *g = SK.group.FindByIdNoOops(SS.TW.shown.group);
                if(!g) break;
                g->color = RgbaColor::FromFloat((float)rgb.x, (float)rgb.y, (float)rgb.z,
                                                g->color.alphaF());

                SS.MarkGroupDirty(g->h);
                SS.GW.ClearSuper();
            } else {
                Error(_("Bad format: specify color as r, g, b"));
            }
            break;
        }
        case Edit::GROUP_OPACITY:
            if(Expr *e = Expr::From(s, /*popUpError=*/true)) {
                double alpha = e->Eval();
                if(alpha < 0 || alpha > 1) {
                    Error(_("Opacity must be between zero and one."));
                } else {
                    Group *g = SK.GetGroup(edit.group);
                    g->color.alpha = (int)(255.1f * alpha);
                    SS.MarkGroupDirty(g->h);
                    SS.GW.ClearSuper();
                }
            }
            break;

        case Edit::TTF_TEXT:
            SS.UndoRemember();
            if(Request *r = SK.request.FindByIdNoOops(edit.request)) {
                r->str = s;
                SS.MarkGroupDirty(r->group);
            }
            break;

        case Edit::STEP_DIM_FINISH:
            if(Expr *e = Expr::From(s, /*popUpError=*/true)) {
                if(stepDim.isDistance) {
                    stepDim.finish = SS.ExprToMm(e);
                } else {
                    stepDim.finish = e->Eval();
                }
            }
            break;

        case Edit::STEP_DIM_STEPS:
            stepDim.steps = min(300, max(1, atoi(s.c_str())));
            break;

        case Edit::TANGENT_ARC_RADIUS:
            if(Expr *e = Expr::From(s, /*popUpError=*/true)) {
                if(e->Eval() < LENGTH_EPS) {
                    Error(_("Radius cannot be zero or negative."));
                    break;
                }
                SS.tangentArcRadius = SS.ExprToMm(e);
            }
            break;

        default: {
            int cnt = 0;
            if(EditControlDoneForStyles(s))         cnt++;
            if(EditControlDoneForConfiguration(s))  cnt++;
            if(EditControlDoneForPaste(s))          cnt++;
            if(EditControlDoneForView(s))           cnt++;
            ssassert(cnt == 1, "Expected exactly one parameter to be edited");
            break;
        }
    }
    SS.GW.Invalidate();
    SS.ScheduleShowTW();

    if(!edit.showAgain) {
        HideEditControl();
        edit.meaning = Edit::NOTHING;
    }
}

} // namespace SolveSpace
