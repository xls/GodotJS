#include "jsb_editor_progress.h"

#include "core/object/message_queue.h"

GodotJSEditorProgress::~GodotJSEditorProgress()
{
    if (progress_) memdelete(progress_);
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
    // Codegen runs overlap (a scene save and a reimport close together) and each passed the same
    // name: ProgressDialog refused the second task, and once the first run ended it, the others
    // stepped and ended a task that was gone ("!tasks.has(p_task)"). Tasks added while the message
    // queue is flushing are refused as well. Give every task its own name, and skip the popup when
    // it would be refused.
    if (MessageQueue::get_singleton()->is_flushing()) return;
    static uint32_t serial = 0;
    progress_ = memnew(EditorProgress(vformat("%s-%d", p_name, ++serial), p_description, p_total_steps));
    progress_->step("", 0);
}

void GodotJSEditorProgress::set_state_name(const String& p_name)
{
    state_name_ = p_name;
}

void GodotJSEditorProgress::set_current(int p_value)
{
    // it runs too slow if p_force_refresh.
    // but be cautious that the internal value of task may not be modified if p_force_refresh is false.
    if (progress_) progress_->step(state_name_, p_value, false);
}

void GodotJSEditorProgress::step()
{
    if (progress_) progress_->step(state_name_, false);
}

void GodotJSEditorProgress::finish()
{
    if (progress_) memdelete(progress_);
    progress_ = nullptr;
}
