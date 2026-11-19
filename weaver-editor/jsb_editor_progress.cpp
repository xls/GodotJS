#include "jsb_editor_progress.h"

GodotJSEditorProgress::~GodotJSEditorProgress()
{
    finish();
}

// EditorProgressBG reaches the status bar through EditorNode, which is gone when a codegen run is
// cut short by quitting: the JS environment frees this object in ScriptServer::finish_languages().
// Then the task is dropped without ending it (its UI is gone with the editor).
static bool _editor_alive()
{
    return EditorNode::get_singleton() != nullptr;
}

void GodotJSEditorProgress::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("init", "name", "description", "total_steps"), &GodotJSEditorProgress::init);
    ClassDB::bind_method(D_METHOD("set_state_name", "name"), &GodotJSEditorProgress::set_state_name);
    ClassDB::bind_method(D_METHOD("set_current", "value"), &GodotJSEditorProgress::set_current);
    ClassDB::bind_method(D_METHOD("step"), &GodotJSEditorProgress::step);
    ClassDB::bind_method(D_METHOD("finish"), &GodotJSEditorProgress::finish);
}

void GodotJSEditorProgress::init(const String& p_name, const String& p_description, int p_total_steps)
{
    jsb_ensure(!progress_);
    // A status bar progress bar, not the modal progress dialog: codegen starts synchronously inside
    // ResourceSaver::save(), scene saves and reimports, and every task step of the dialog runs a
    // nested Main::iteration(). Inside a script that saves a resource, that iteration reloaded
    // scripts changed on disk (a function of the reloaded script still running returned into freed
    // memory) and instanced scenes (freeing an AnimationPlayer the scene save still held). The
    // background progress only queues deferred UI updates.
    // Every task gets its own name: codegen runs overlap (a scene save and a reimport close
    // together), and with one shared name the first run to finish ended the task of the others.
    if (!_editor_alive()) return;
    static uint32_t serial = 0;
    progress_ = memnew(EditorProgressBG(vformat("%s-%d", p_name, ++serial), p_description, p_total_steps));
    progress_->step(0);
}

void GodotJSEditorProgress::set_state_name(const String& p_name)
{
    state_name_ = p_name;
}

void GodotJSEditorProgress::set_current(int p_value)
{
    // The status bar shows no state name; state_name_ is kept for the binding.
    if (progress_ && _editor_alive()) progress_->step(p_value);
}

void GodotJSEditorProgress::step()
{
    if (progress_ && _editor_alive()) progress_->step();
}

void GodotJSEditorProgress::finish()
{
    if (!progress_) return;
    if (_editor_alive())
    {
        memdelete(progress_);
    }
    else
    {
        // Skip ~EditorProgressBG (it would end the task through the null EditorNode).
        progress_->task.~String();
        memfree(progress_);
    }
    progress_ = nullptr;
}
