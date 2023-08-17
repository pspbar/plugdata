/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class SubpatchObject final : public TextBase {

    pd::Patch::Ptr subpatch;
    Value isGraphChild = SynchronousValue(var(false));
    Value hideNameAndArgs = SynchronousValue(var(false));

    bool locked = false;

public:
    SubpatchObject(void* obj, Object* object)
        : TextBase(obj, object)
        , subpatch(new pd::Patch(obj, cnv->pd, false))
    {
        object->hvccMode.addListener(this);

        if (getValue<bool>(object->hvccMode)) {
            checkHvccCompatibility(getText(), subpatch.get());
        }

        objectParameters.addParamBool("Is graph", cGeneral, &isGraphChild, { "No", "Yes" });
        objectParameters.addParamBool("Hide name and arguments", cGeneral, &hideNameAndArgs, { "No", "Yes" });
    }

    ~SubpatchObject() override
    {
        object->hvccMode.removeListener(this);
        closeOpenedSubpatchers();
    }

    void update() override
    {
        isGraphChild = static_cast<bool>(subpatch->getPointer()->gl_isgraph);
        hideNameAndArgs = static_cast<bool>(subpatch->getPointer()->gl_hidetext);

        updateValue();
    }

    void updateValue()
    {
        // Change from subpatch to graph
        if (auto canvas = ptr.get<t_canvas>()) {
            if (canvas->gl_isgraph) {
                cnv->setSelected(object, false);
                object->cnv->editor->sidebar->hideParameters();
                object->setType(objectText, canvas.get());
            }
        }
    };

    void mouseDown(MouseEvent const& e) override
    {
        if(!e.mods.isLeftButtonDown()) return;
        
        if (locked && click(e.getPosition(), e.mods.isShiftDown(), e.mods.isAltDown())) {
            return;
        }

        //  If locked and it's a left click
        if (locked && !e.mods.isRightButtonDown() && !object->attachedToMouse) {
            openSubpatch();

            return;
        } else {
            TextBase::mouseDown(e);
        }
    }

    // Most objects ignore mouseclicks when locked
    // Objects can override this to do custom locking behaviour
    void lock(bool isLocked) override
    {
        locked = isLocked;
    }

    pd::Patch::Ptr getPatch() override
    {
        return subpatch;
    }

    void checkGraphState()
    {
        int isGraph = getValue<bool>(isGraphChild);
        int hideText = getValue<bool>(hideNameAndArgs);

        if (auto glist = ptr.get<t_glist>()) {
            canvas_setgraph(glist.get(), isGraph + 2 * hideText, 0);
        }
        repaint();

        MessageManager::callAsync([this, _this = SafePointer(this)]() {
            if (!_this)
                return;

            // Change from subpatch to graph
            if (auto glist = ptr.get<t_glist>()) {
                if (glist->gl_isgraph) {
                    cnv->setSelected(object, false);
                    object->cnv->editor->sidebar->hideParameters();
                    object->setType(getText(), glist.get());
                    return;
                }
            }
        });
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(isGraphChild) || v.refersToSameSourceAs(hideNameAndArgs)) {
            checkGraphState();
        } else if (v.refersToSameSourceAs(object->hvccMode)) {
            if (getValue<bool>(v)) {
                checkHvccCompatibility(getText(), subpatch.get());
            }
        }
    }

    bool canOpenFromMenu() override
    {
        return true;
    }

    void openFromMenu() override
    {
        openSubpatch();
    }

    static void checkHvccCompatibility(const String& objectText, pd::Patch::Ptr patch, String const& prefix = "")
    {
        auto* instance = patch->instance;
        
        if(objectText.startsWith("pd @hv_obj"))
        {
            return;
        }
        
        // Table is really a subpatch, but it is supported in hvcc as an object
        // So if we find a table, just accept it
        if(canvas_istable(static_cast<t_canvas*>(patch->getPointer().get()))) return;

        for (auto* object : patch->getObjects()) {
            const String name = libpd_get_object_class_name(object);

            if (name == "canvas" || name == "graph") {
                pd::Patch::Ptr subpatch = new pd::Patch(object, instance, false);
                
                char* text = nullptr;
                int size = 0;
                libpd_get_object_text(object, &text, &size);
                auto objName = String::fromUTF8(text, size);
                
                checkHvccCompatibility(objName, subpatch, prefix + objName + " -> ");
                freebytes(static_cast<void*>(text), static_cast<size_t>(size) * sizeof(char));

            } else if (!Object::hvccObjects.contains(name)) {
                instance->logWarning(String("Warning: object \"" + prefix + name + "\" is not supported in Compiled Mode"));
            }
        }
    }
};
