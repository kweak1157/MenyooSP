/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "SpoonerTaskSequence.h"

#include "..\..\macros.h"

//#include "STSTask.h"
#include "STSTasks.h"

//#include "..\..\Menu\Menu.h"
//#include "..\..\Menu\Routine.h"

//#include "..\..\Natives\natives2.h"

#include "..\..\Util\Clock.h"

#include <memory>
#include <utility>
#include <vector>

namespace sub::Spooner
{
	SpoonerTaskSequence::SpoonerTaskSequence()
	{
		this->bJustJumpedToNext = false;
		this->progress = -1;
		this->timer = GetTickMs();
	}

	SpoonerTaskSequence::SpoonerTaskSequence(const SpoonerTaskSequence& right)
	{
		*this = right;
	}

	SpoonerTaskSequence& SpoonerTaskSequence::operator=(const SpoonerTaskSequence& right)
	{
		if (this == &right)
			return *this;

		this->Reset(true);
		this->bJustJumpedToNext = right.bJustJumpedToNext;
		this->progress = right.progress;
		this->timer = right.timer;
		for (auto& tskPtr : right.tasks)
		{
			if (tskPtr != nullptr)
			{
				STSTask* newtskPtr = this->AddTask(tskPtr->type);
				if (newtskPtr != nullptr)
					newtskPtr->Assign(tskPtr.get());
			}
		}
		return *this;
	}

	bool SpoonerTaskSequence::ContainsType(const STSTaskType& value)
	{
		for (auto& t : this->tasks)
		{
			if (t != nullptr)
			{
				if (t->type == value)
					return true;
			}
		}
		return false;
	}

	UINT16 SpoonerTaskSequence::TaskCount() const
	{
		return (UINT16)this->tasks.size();
	}
	bool SpoonerTaskSequence::empty() const
	{
		return this->tasks.empty();
	}

	void SpoonerTaskSequence::AddTask(std::unique_ptr<STSTask> tskPtr)
	{
		this->tasks.push_back(std::move(tskPtr));
	}
	STSTask* SpoonerTaskSequence::AddTask(const STSTaskType& ofType)
	{
		std::unique_ptr<STSTask> tskPtr;
		switch (ofType)
		{
		case STSTaskType::Nothing: tskPtr = std::make_unique<STSTasks::Nothing>(); break;
		case STSTaskType::SetHealth: tskPtr = std::make_unique<STSTasks::SetHealth>(); break;
		case STSTaskType::AddBlip: tskPtr = std::make_unique<STSTasks::AddBlip>(); break;
		case STSTaskType::RemoveBlip: tskPtr = std::make_unique<STSTasks::RemoveBlip>(); break;
		case STSTaskType::Pause: tskPtr = std::make_unique<STSTasks::Pause>(); break;
		case STSTaskType::UsePhone: tskPtr = std::make_unique<STSTasks::UsePhone>(); break;
		case STSTaskType::ThrowProjectile: tskPtr = std::make_unique<STSTasks::ThrowProjectile>(); break;
		case STSTaskType::Writhe: tskPtr = std::make_unique<STSTasks::Writhe>(); break;
		case STSTaskType::FaceDirection: tskPtr = std::make_unique<STSTasks::FaceDirection>(); break;
		case STSTaskType::FaceEntity: tskPtr = std::make_unique<STSTasks::FaceEntity>(); break;
		case STSTaskType::LookAtCoord: tskPtr = std::make_unique<STSTasks::LookAtCoord>(); break;
		case STSTaskType::LookAtEntity: tskPtr = std::make_unique<STSTasks::LookAtEntity>(); break;
		case STSTaskType::TeleportToCoord: tskPtr = std::make_unique<STSTasks::TeleportToCoord>(); break;
		case STSTaskType::SeekCoverAtCoord: tskPtr = std::make_unique<STSTasks::SeekCoverAtCoord>(); break;
		case STSTaskType::SlideToCoord: tskPtr = std::make_unique<STSTasks::SlideToCoord>(); break;
		case STSTaskType::GoToCoord: tskPtr = std::make_unique<STSTasks::GoToCoord>(); break;
		case STSTaskType::FollowRoute: tskPtr = std::make_unique<STSTasks::FollowRoute>(); break;
		case STSTaskType::FollowEntity: tskPtr = std::make_unique<STSTasks::FollowEntity>(); break;
		case STSTaskType::PatrolInRange: tskPtr = std::make_unique<STSTasks::PatrolInRange>(); break;
		case STSTaskType::WanderFreely: tskPtr = std::make_unique<STSTasks::WanderFreely>(); break;
		case STSTaskType::FleeFromCoord: tskPtr = std::make_unique<STSTasks::FleeFromCoord>(); break;
		case STSTaskType::NearestAppropriateAction: tskPtr = std::make_unique<STSTasks::NearestAppropriateAction>(); break;
		case STSTaskType::ScenarioAction: tskPtr = std::make_unique<STSTasks::ScenarioAction>(); break;
		case STSTaskType::PlayAnimation: tskPtr = std::make_unique<STSTasks::PlayAnimation>(); break;
		case STSTaskType::SetActiveWeapon: tskPtr = std::make_unique<STSTasks::SetActiveWeapon>(); break;
		case STSTaskType::AimAtCoord: tskPtr = std::make_unique<STSTasks::AimAtCoord>(); break;
		case STSTaskType::AimAtEntity: tskPtr = std::make_unique<STSTasks::AimAtEntity>(); break;
		case STSTaskType::ShootAtCoord: tskPtr = std::make_unique<STSTasks::ShootAtCoord>(); break;
		case STSTaskType::ShootAtEntity: tskPtr = std::make_unique<STSTasks::ShootAtEntity>(); break;
		case STSTaskType::FightHatedTargets: tskPtr = std::make_unique<STSTasks::FightHatedTargets>(); break;
		case STSTaskType::FightPed: tskPtr = std::make_unique<STSTasks::FightPed>(); break;
		case STSTaskType::SpeakToPed: tskPtr = std::make_unique<STSTasks::SpeakToPed>(); break;
		case STSTaskType::PlaySpeechWithVoice: tskPtr = std::make_unique<STSTasks::PlaySpeechWithVoice>(); break;

		case STSTaskType::WarpIntoVehicle: tskPtr = std::make_unique<STSTasks::WarpIntoVehicle>(); break;
		case STSTaskType::EnterVehicle: tskPtr = std::make_unique<STSTasks::EnterVehicle>(); break;
		case STSTaskType::ExitVehicle: tskPtr = std::make_unique<STSTasks::ExitVehicle>(); break;
		case STSTaskType::DriveWander: tskPtr = std::make_unique<STSTasks::DriveWander>(); break;
		case STSTaskType::DriveToCoord: tskPtr = std::make_unique<STSTasks::DriveToCoord>(); break;
		case STSTaskType::DriveFollowEntity: tskPtr = std::make_unique<STSTasks::DriveFollowEntity>(); break;
		case STSTaskType::DriveLandPlane: tskPtr = std::make_unique<STSTasks::DriveLandPlane>(); break;

		case STSTaskType::AchieveVehicleForwardSpeed: tskPtr = std::make_unique<STSTasks::AchieveVehicleForwardSpeed>(); break;
		case STSTaskType::EmptyVehicle: tskPtr = std::make_unique<STSTasks::EmptyVehicle>(); break;

		case STSTaskType::ChangeTextureVariation: tskPtr = std::make_unique<STSTasks::ChangeTextureVariation>(); break;

		case STSTaskType::AchieveVelocity: tskPtr = std::make_unique<STSTasks::AchieveVelocity>(); break;
		case STSTaskType::AchievePushForce: tskPtr = std::make_unique<STSTasks::AchievePushForce>(); break;
		case STSTaskType::OscillateToPoint: tskPtr = std::make_unique<STSTasks::OscillateToPoint>(); break;
		case STSTaskType::OscillateToEntity: tskPtr = std::make_unique<STSTasks::OscillateToEntity>(); break;
		case STSTaskType::FreezeInPlace: tskPtr = std::make_unique<STSTasks::FreezeInPlace>(); break;
		case STSTaskType::SetRotation: tskPtr = std::make_unique<STSTasks::SetRotation>(); break;
		case STSTaskType::ChangeOpacity: tskPtr = std::make_unique<STSTasks::ChangeOpacity>(); break;
		case STSTaskType::TriggerFx: tskPtr = std::make_unique<STSTasks::TriggerFx>(); break;

		case STSTaskType::SnapTasks: tskPtr = std::make_unique<STSTasks::SnapTasks>(); break;
		case STSTaskType::EndSequence: tskPtr = std::make_unique<STSTasks::EndSequence>(); break;
		}

		STSTask* raw = tskPtr.get();
		if (tskPtr != nullptr)
			this->tasks.push_back(std::move(tskPtr));
		return raw;
	}

	void SpoonerTaskSequence::RemoveTask(UINT16 index)
	{
		if (this->tasks.size() < 2) // 1 or 0
		{
			this->Reset(true);
		}
		else
		{
			this->tasks.erase(this->tasks.begin() + index);
		}
	}

	void SpoonerTaskSequence::SwapTasks(UINT16 index1, UINT16 index2)
	{
		if (index1 >= 0 && index1 < this->tasks.size()
			&& index2 >= 0 && index2 < this->tasks.size())
		{
			std::swap(this->tasks[index1], this->tasks[index2]);
		}
	}

	std::vector<std::unique_ptr<STSTask>>& SpoonerTaskSequence::AllTasks()
	{
		return this->tasks;
	}

	bool SpoonerTaskSequence::IsActive() const
	{
		return this->progress >= 0;
	}
	void SpoonerTaskSequence::Start()
	{
		if (!this->tasks.empty())
		{
			this->bJustJumpedToNext = true;
			this->progress = 0;
			this->timer = GetTickMs() + this->tasks[0]->duration;
		}
	}
	void SpoonerTaskSequence::Reset(bool deleteTasks)
	{
		///CLEAR_PED_TASKS_IMMEDIATELY(ep.Handle()); // NO
		//this->bJustJumpedToNext = true;
		this->progress = -1;
		//this->timer = GetTickCount();
		if (deleteTasks)
		{
			this->tasks.clear();
		}
	}
	void SpoonerTaskSequence::Tick(void* ev)
	{
		/*Game::Print::setupdraw(GTAfont::Arial, Vector2(0.34f, 0.34f), false, true, true);
		Game::Print::drawstring(oss_ << "Task count = " << (int)this->tasks.size(), 0.9f, 0.23f);
		Game::Print::setupdraw(GTAfont::Arial, Vector2(0.34f, 0.34f), false, true, true);
		Game::Print::drawstring(oss_ << "Task index = " << (int)this->progress, 0.9f, 0.2f);
		Game::Print::setupdraw(GTAfont::Arial, Vector2(0.34f, 0.34f), false, true, true);
		Game::Print::drawstring(oss_ << "Just Jumped = " << (int)this->bJustJumpedToNext, 0.9f, 0.26f);
		if (this->progress < this->tasks.size())
		{
		Game::Print::setupdraw(GTAfont::Arial, Vector2(0.34f, 0.34f), false, true, true);
		Game::Print::drawstring(oss_ << "Current task duration = " << (int)this->tasks[this->progress]->duration, 0.9f, 0.29f);
		}
		Game::Print::setupdraw(GTAfont::Arial, Vector2(0.34f, 0.34f), false, true, true);
		Game::Print::drawstring(oss_ << "Timer = " << (int)this->timer, 0.9f, 0.32f);
		Game::Print::setupdraw(GTAfont::Arial, Vector2(0.34f, 0.34f), false, true, true);
		Game::Print::drawstring(oss_ << "TickCount = " << (int)GetTickCount(), 0.9f, 0.35f);*/

		if (this->progress < 0 || this->tasks.empty())
		{
			return;
			//goto label_returnPls;
		}
		else if (this->progress >= this->tasks.size()) { this->progress = 0; }

		STSTask* task = this->tasks[this->progress].get();

		if (this->bJustJumpedToNext || task->isLoopedTask)
		{
			task->Run(ev);
			this->bJustJumpedToNext = false;
			if (task->type == STSTaskType::EndSequence)
			{
				this->Reset();
				return;
				//goto label_returnPls;
			}
		}

		if (GetTickMs() > this->timer)
		{
			if (task->durationAfterLife <= 0)
				task->End(ev);
			INT16 nextTaskIndex = (this->progress + 1 >= this->tasks.size()) ?
				0 : this->progress + 1;
			STSTask* nextTask = this->tasks[nextTaskIndex].get();
			this->bJustJumpedToNext = true;
			this->progress++;
			this->timer = GetTickMs() + (nextTask->duration);
		}

		if (this->progress >= this->tasks.size())
		{
			this->Start(); // Repeat
		}

		//label_returnPls:;
	}

}



