/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#pragma once

#include "STSTask.h"

#include <cstdint>
#include <memory>
#include <vector>

typedef short INT16;
typedef unsigned short UINT16;
typedef unsigned long DWORD;

namespace sub::Spooner
{
	class SpoonerTaskSequence final
	{
	private:
		bool bJustJumpedToNext;
		std::vector<std::unique_ptr<STSTask>> tasks;
		INT16 progress;
		std::uint64_t timer;
	public:
		SpoonerTaskSequence();
		SpoonerTaskSequence(const SpoonerTaskSequence& right);
		SpoonerTaskSequence(SpoonerTaskSequence&& right) noexcept = default;
		SpoonerTaskSequence& operator=(const SpoonerTaskSequence& right);
		SpoonerTaskSequence& operator=(SpoonerTaskSequence&& right) noexcept = default;

		template<typename R = STSTask> inline R* GetTask(UINT16 index)
		{
			return reinterpret_cast<R*>(this->tasks[index].get());
		}

		bool ContainsType(const STSTaskType& value);

		UINT16 TaskCount() const;
		bool empty() const;

		void AddTask(std::unique_ptr<STSTask> tskPtr);
		STSTask* AddTask(const STSTaskType& ofType);

		void RemoveTask(UINT16 index);

		void SwapTasks(UINT16 index1, UINT16 index2);

		std::vector<std::unique_ptr<STSTask>>& AllTasks();

		bool IsActive() const;
		void Start();
		void Reset(bool deleteTasks = false);
		void Tick(void* ev);
	};

	/// Spooner ped options sub goes to task sequence (taskList) submenu.
	// In taskList submenu, show name of each task in AllTasks() and have ADD NEW option at bottom. Should be a START[Start()] / STOP[Reset(false) and ClearPedTasksImm] button right below it.
	/// AddNew submenu should name all tasks and pressing should take back to taskList submenu with added task.
	// X/B to delete selected task. Show in button instructions on the bottom right.
	/// Press task for task settings. Settings should be a [one to many funcs] submenu with 'Duration' in the main submenu func. Custom appropriate settings in each sub.
	// Save to Spooner xml k

}



