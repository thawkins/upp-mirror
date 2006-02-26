#include "SqlCtrl.h"

void SqlArray::Join(SqlId id, ArrayCtrl& master) {
	master.AddCtrlAt(0, *this);
	fk = id;
}

void SqlArray::Join(ArrayCtrl& master) {
	Join(master.GetKeyId(), master);
}

void SqlArray::Clear() {
	fkv = Null;
	lateinsert = false;
	ArrayCtrl::Clear();
}

void SqlArray::Reset() {
	ArrayCtrl::Reset();
	fk = SqlId();
}

bool SqlArray::CanInsert() const {
	return fk.IsNull() || !IsNull(fkv);
}

bool SqlArray::PerformInsert() {
	ASSERT(IsCursor());
	SqlInsert insert(table);
	for(int i = 0; i < GetIndexCount(); i++)
		if(!GetId(i).IsNull())
			insert(GetId(i), Get(i));
	if(!fk.IsNull())
		insert(fk, fkv);
	Session().ClearError();
	Session().Begin();
	Sql cursor(Session());
	cursor * insert;
	return OkCommit(Session(), t_("Can't insert record."));
}

bool SqlArray::PerformDelete() {
	ASSERT(IsCursor());
	Session().ClearError();
	if(IsInsert() && lateinsert) {
		lateinsert = false;
		return true;
	}
	Session().Begin();
	Sql cursor(Session());
	cursor * Delete(table)
	         .Where(fk.IsNull() ? SqlId(GetKeyId()) == GetKey()
	                            : SqlId(GetKeyId()) == GetKey() && fk == fkv);

	return OkCommit(Session(), t_("Can't delete record."));
}

bool SqlArray::UpdateRow() {
	if(IsInsert() && lateinsert) {
		if(!PerformInsert()) return false;
	}
	else {
		SqlUpdate update(table);
		for(int i = 0; i < GetIndexCount(); i++)
			if(!GetId(i).IsNull() && IsModified(i))
				update(GetId(i), Get(i));
		if(update) {
			Session().ClearError();
			Session().Begin();
			Sql cursor(Session());
			cursor * update
			         .Where(fk.IsNull() ? SqlId(GetKeyId()) == GetOriginalKey()
			                            : SqlId(GetKeyId()) == GetOriginalKey() && fk == fkv);
			if(!OkCommit(Session(), t_("Can't update record.")))
				return false;
		}
	}
	lateinsert = false;
	return ArrayCtrl::UpdateRow();
}

void SqlArray::RejectRow() {
	if(IsInsert())
		PerformDelete();
	ArrayCtrl::RejectRow();
}

void SqlArray::StartInsert() {
	if(!CanInsert()) return;
	DoAppend();
	if(IsNull(GetKey()))
		lateinsert = true;
	else
		PerformInsert();
}

void SqlArray::StartDuplicate() {
	if(!IsCursor() || !CanInsert()) return;
	Vector<Value> v;
	int i;
	for(i = 0; i < GetIndexCount(); i++)
		v.Add(Get(i));
	if(!KillCursor()) return;
	StartInsert();
	for(i = 0; i < GetIndexCount(); i++)
		if(IsNull(Get(i)) && i < v.GetCount())
			Set(i, v[i]);
}

void SqlArray::SetData(const Value& v) {
	fkv = v;
	if(fk.IsNull()) {
		CHECK(KillCursor());
		Clear();
	}
	else
		Query();
}

Value SqlArray::GetData() const {
	return fkv;
}

bool  SqlArray::Accept() {
	bool b = ArrayCtrl::Accept();
	Ctrl::ClearModify();
	return b;
}

void SqlArray::DoRemove() {
	if(!IsCursor() || IsAskRemove() && !PromptOKCancel(RowFormat(t_("Do you really want to delete the selected %s ?")))) return;
	if(PerformDelete()) {
		Remove(GetCursor());
		WhenArrayAction();
	}
}

void SqlArray::StdBar(Bar& menu) {
	bool c = !IsEdit();
	bool d = c && IsCursor();
	if(IsAppending() || IsInserting())
		menu.Add(c && CanInsert(), RowFormat(t_("Insert %s")), THISBACK(StartInsert))
			.Help(RowFormat(t_("Insert a new %s into the table.")))
			.Key(K_INSERT);
	if(IsDuplicating())
		menu.Add(d && CanInsert(), RowFormat(t_("Copy %s")), THISBACK(StartDuplicate))
			.Help(RowFormat(t_("Duplicate current table %s.")))
			.Key(K_CTRL_INSERT);
	if(IsEditing())
		menu.Add(d, RowFormat(t_("Edit %s")), THISBACK(DoEdit))
			.Help(RowFormat(t_("Edit active %s.")))
			.Key(K_CTRL_ENTER);
	if(IsRemoving())
		menu.Add(d, RowFormat(t_("Delete %s\tDelete")), THISBACK(DoRemove))
			.Help(RowFormat(t_("Delete active %s.")))
			.Key(K_DELETE);
}

void SqlArray::AppendQuery(SqlBool where)
{
	lateinsert = false;
	if(fk.IsNull() || !IsNull(fkv)) {
		SqlSet cols;
		for(int i = 0; i < GetIndexCount(); i++)
			if(!GetId(i).IsNull())
				cols.Cat(SqlId(GetId(i)));
		SqlBool wh = where;
		if(!fk.IsNull())
			wh = wh && fk == fkv;
		Sql sql(Session());
		Session().ClearError();
//		AutoWaitCursor awc(querytime); // todo: Fidler -> WaitCursor
		sql * ::Select(cols).From(table).Where(wh).OrderBy(orderby);
		if(ShowError(sql)) {
//			awc.Cancel(); // todo: Fidler -> WaitCursor
			return;
		}
		for(;;) {
			Vector<Value> row;
			if(!sql.Fetch(row)) break;
			Add(row);
		}
		if(GetCount())
			if(goendpostquery)
				GoEnd();
			else
				GoBegin();
		WhenPostQuery();
	}
}

void SqlArray::Query() {
	CHECK(KillCursor());
	ArrayCtrl::Clear();
	lateinsert = false;
	AppendQuery(where);
}

SqlArray::SqlArray() {
	querytime = 0;
	ssn = NULL;
	WhenBar = THISBACK(StdBar);
	goendpostquery = false;
	lateinsert = false;
	RowName(t_("record"));
}
