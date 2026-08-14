/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "MarkerManagement.h"

#include "..\..\macros.h"

//#include "..\..\Menu\Menu.h"
//#include "..\..\Menu\Routine.h"

#include "..\..\Natives\natives2.h"
#include "..\..\Util\GTAmath.h"
#include "..\..\Scripting\GTAentity.h"
#include "..\..\Scripting\World.h"
#include "..\..\Scripting\GameplayCamera.h"
#include "..\..\Scripting\Raycast.h"
#include "..\..\Scripting\Game.h"
#include "..\..\Scripting\enums.h"
#include "..\..\Scripting\GTAvehicle.h"
#include "..\..\Scripting\Camera.h"

#include "Databases.h"
#include "SpoonerMarker.h"

#include <string>
#include <vector>

namespace sub::Spooner
{
	namespace MarkerManagement
	{
		namespace
		{
			enum class MarkerTeleportPhase : UINT8
			{
				Idle = 0,
				FadeOut,
				AfterTeleport
			};

			MarkerTeleportPhase s_tpPhase = MarkerTeleportPhase::Idle;
			int s_tpPhaseStartMs = 0;
			int s_tpEntityHandle = 0;
			Vector3 s_tpFinalDest;
			float s_tpFinalDestHeading = 0.0f;
			bool s_tpIsVehicle = false;

			void ApplyMarkerTeleport()
			{
				GTAentity entityToTeleport(s_tpEntityHandle);
				if (!entityToTeleport.Exists())
				{
					DO_SCREEN_FADE_IN(300);
					s_tpPhase = MarkerTeleportPhase::Idle;
					return;
				}

				entityToTeleport.RequestControl();
				entityToTeleport.Position_set(s_tpFinalDest);
				entityToTeleport.Heading_set(s_tpFinalDestHeading);
				if (s_tpIsVehicle)
					GTAvehicle(entityToTeleport).Speed_set(4.2f); // Approx 15 km/h
				GameplayCamera::RelativeHeading_set(0.0f);
				GameplayCamera::RelativePitch_set(0.0f);
			}

			void TickMarkerTeleport()
			{
				if (s_tpPhase == MarkerTeleportPhase::Idle)
					return;

				const int now = GET_GAME_TIMER();
				const int elapsed = now - s_tpPhaseStartMs;

				if (s_tpPhase == MarkerTeleportPhase::FadeOut)
				{
					if (elapsed >= 20)
					{
						ApplyMarkerTeleport();
						s_tpPhase = MarkerTeleportPhase::AfterTeleport;
						s_tpPhaseStartMs = now;
					}
				}
				else if (s_tpPhase == MarkerTeleportPhase::AfterTeleport)
				{
					if (elapsed >= 100)
					{
						DO_SCREEN_FADE_IN(300);
						s_tpPhase = MarkerTeleportPhase::Idle;
					}
				}
			}

			void BeginMarkerTeleport(GTAentity& entityToTeleport, const Vector3& finalDest, float finalDestHeading)
			{
				SpoonerMarker::bPlayerJustTeleportedBetweenMarkers = true;
				DO_SCREEN_FADE_OUT(5);
				entityToTeleport.RequestControl();

				s_tpEntityHandle = entityToTeleport.Handle();
				s_tpFinalDest = finalDest;
				s_tpFinalDestHeading = finalDestHeading;
				s_tpIsVehicle = ((EntityType)entityToTeleport.Type() == EntityType::VEHICLE);
				s_tpPhase = MarkerTeleportPhase::FadeOut;
				s_tpPhaseStartMs = GET_GAME_TIMER();
			}
		}

		void DrawAll()
		{
			TickMarkerTeleport();

			GTAentity myPed = PLAYER_PED_ID();
			GTAentity myVehicle = GET_VEHICLE_PED_IS_IN(myPed.Handle(), false);
			GTAentity* entityToTeleport = &myPed;
			bool bMyPedIsInVehicle = myVehicle.Exists();

			const Vector3& myDim1 = myPed.Dim1();
			Vector3 myVehicleDim1;

			std::vector<Vector3> vCoordsToCheckPed
			{
				myPed.Position_get(),
				//myPed.GetOffsetInWorldCoords(0, myDim1.y, -(myDim1.z / 2)),
				//myPed.GetOffsetInWorldCoords(0, -myDim1.y, -(myDim1.z / 2)),
				//myPed.GetOffsetInWorldCoords(myDim1.x, 0, -(myDim1.z / 2)),
				//myPed.GetOffsetInWorldCoords(-myDim1.x, 0, -(myDim1.z / 2)),
			};
			std::vector<Vector3> vCoordsToCheckVehicle;
			auto* vCoordsToCheck = &vCoordsToCheckPed;

			if (bMyPedIsInVehicle)
			{
				myVehicleDim1 = myVehicle.Dim1();

				vCoordsToCheckVehicle =
				{
					myVehicle.Position_get(),
					myVehicle.GetOffsetInWorldCoords(0, myVehicleDim1.y, -(myVehicleDim1.z / 2)),
					myVehicle.GetOffsetInWorldCoords(0, -myVehicleDim1.y, -(myVehicleDim1.z / 2)),
					myVehicle.GetOffsetInWorldCoords(myVehicleDim1.x, 0, -(myVehicleDim1.z / 2)),
					myVehicle.GetOffsetInWorldCoords(-myVehicleDim1.x, 0, -(myVehicleDim1.z / 2)),
				};
			}

			const auto& renderingCam = World::RenderingCamera_get();
			const Vector3& camPos = (renderingCam == 0 ? GameplayCamera::Position_get() : renderingCam.Position_get());

			for (auto& marker : Databases::MarkerDb)
			{
				entityToTeleport = &myPed;
				vCoordsToCheck = &vCoordsToCheckPed;
				if (marker.m_allowVehicles && bMyPedIsInVehicle)
				{
					entityToTeleport = &myVehicle;
					vCoordsToCheck = &vCoordsToCheckVehicle;
				}

				Vector3 finalPosition, finalDirection, finalRotation;
				if (marker.m_attachmentArgs.attachedTo.Exists())
				{
					finalPosition = marker.m_attachmentArgs.attachedTo.GetOffsetInWorldCoords(marker.m_attachmentArgs.offset);
					marker.m_position = finalPosition; // If detached, stay at pos
					const Vector3& entRot = marker.m_attachmentArgs.attachedTo.Rotation_get();
					//finalDirection = Vector3::DirectionToRotation(entRot);
					finalRotation = Vector3(0, 0, entRot.z) + marker.m_attachmentArgs.rotation;
				}
				else
				{
					finalPosition = marker.m_position;
					//finalDirection = Vector3();
					finalRotation = marker.m_rotation;
				}

				bool bSelectedInSub = marker.m_selectedInSub;
				marker.m_selectedInSub = false;
				if (vCoordsToCheck->front().DistanceTo(finalPosition) < 80.0f)
					World::DrawMarker(marker.m_type, finalPosition, finalDirection, finalRotation, (Vector3::One() * marker.m_scale), bSelectedInSub ? RGBA(marker.m_colour.Inverse(false), 240) : marker.m_colour, bSelectedInSub, false, 2, marker.m_rotateContinuously, std::string(), std::string(), false);

				if (marker.m_showName)
				{
					const auto& ray = RaycastResult::Raycast(camPos, finalPosition + (Vector3::Normalize(finalPosition - camPos) * Vector3(0, -marker.m_scale, 0)), IntersectOptions::Everything);
					if (!ray.DidHitAnything() && vCoordsToCheck->front().DistanceTo(finalPosition) < 40.0f)
					{
						Vector2 scrnPos;
						if (World::WorldToScreen(finalPosition, scrnPos))
						{
							Game::Print::setupdraw(GTAfont::Impact, Vector2(0.3f, 0.3f), true, false, true);
							Game::Print::drawstring(marker.m_name, scrnPos.x, scrnPos.y);
						}
					}
				}

				if (s_tpPhase != MarkerTeleportPhase::Idle)
					continue;

				if (!SpoonerMarker::bPlayerJustTeleportedBetweenMarkers)
				{
					SpoonerMarkerPosition* dest = marker.m_destinationPtr == nullptr ? &marker.m_destinationVal : marker.m_destinationPtr;

					if (!dest->m_position.IsZero())
					{
						if (!bMyPedIsInVehicle || marker.m_allowVehicles)
						{
							for (auto& c : *vCoordsToCheck)
							{
								if (c.DistanceTo(finalPosition) < marker.m_scale)
								{
									Vector3 finalDest;
									float finalDestHeading;
									if (dest->m_attachmentArgs.attachedTo.Exists())
									{
										finalDest = dest->m_attachmentArgs.attachedTo.GetOffsetInWorldCoords(dest->m_attachmentArgs.offset);
										finalDestHeading = dest->m_attachmentArgs.attachedTo.Rotation_get().z + marker.m_destinationHeading;
									}
									else
									{
										finalDest = dest->m_position;
										finalDestHeading = marker.m_destinationHeading;
									}

									BeginMarkerTeleport(*entityToTeleport, finalDest, finalDestHeading);
									return;
								}
							}
						}
					}
				}
				else
				{
					SpoonerMarker::bPlayerJustTeleportedBetweenMarkers = false;
					for (auto& mrkr : Databases::MarkerDb)
					{
						for (auto& c : *vCoordsToCheck)
						{
							Vector3 finalPosition = mrkr.m_attachmentArgs.attachedTo.Exists() ? mrkr.m_attachmentArgs.attachedTo.GetOffsetInWorldCoords(mrkr.m_attachmentArgs.offset) : mrkr.m_position;
							if (c.DistanceTo(finalPosition) < mrkr.m_scale + 0.36f)
							{
								SpoonerMarker::bPlayerJustTeleportedBetweenMarkers = true;
								return;
							}
						}
					}

				}
			}

		}

		SpoonerMarker* AddMarker(const std::string& name, const Vector3& position, const Vector3& rotation)
		{
			Databases::MarkerDb.push_back(SpoonerMarker(name, position, rotation));
			return &Databases::MarkerDb.back();
		}
		SpoonerMarker* AddMarker(const Vector3& position, const Vector3& rotation)
		{
			std::string inputStr = Game::InputBox("~`", 26U, "Enter custom marker name:");
			if (inputStr.compare("~`") != 0)
			{
				Databases::MarkerDb.push_back(SpoonerMarker(inputStr, position, rotation));
				return &Databases::MarkerDb.back();
			}
			else
				return nullptr;
			/*OnscreenKeyboard::State::Set(OnscreenKeyboard::Purpose::SpoonerAddMarker, "~`", 25U, "Enter custom marker name");
			OnscreenKeyboard::State::arg1._vec3 = new Vector3(position);
			OnscreenKeyboard::State::arg2._vec3 = new Vector3(rotation);
			OnscreenKeyboard::State::arg3._bool = atSpoonerModeTarget;
			return nullptr;*/
			//Databases::MarkerDb.push_back(SpoonerMarker("NO_NAME", position, rotation));
			//return &Databases::MarkerDb.back();
		}

		void GetAllMarkersInRange(std::vector<SpoonerMarker>& result, const Vector3& position, float radius)
		{
			result.clear();
			for (auto& marker : Databases::MarkerDb)
			{
				if (position.DistanceTo(marker.m_position) < radius)
				{
					result.push_back(marker);
				}
			}
		}

		void ClearDb()
		{
			Databases::MarkerDb.clear();
		}
		inline std::vector<SpoonerMarker>::iterator RemoveMarker(std::vector<SpoonerMarker>::iterator it)
		{
			for (auto& x : Databases::MarkerDb)
			{
				if (x.m_destinationPtr == &*it)
					x.m_destinationPtr = nullptr;
			}
			return Databases::MarkerDb.erase(it);
		}
		void RemoveMarker(SpoonerMarker& marker)
		{
			auto it = std::find(Databases::MarkerDb.begin(), Databases::MarkerDb.end(), marker);
			if (it != Databases::MarkerDb.end())
			{
				RemoveMarker(it);
			}
		}
		void RemoveMarker(int indexInDb)
		{
			if (indexInDb >= 0 && indexInDb < Databases::MarkerDb.size())
			{
				RemoveMarker(Databases::MarkerDb.begin() + indexInDb);
			}
		}
		void RemoveAllMarkers()
		{
			MarkerManagement::ClearDb();
		}
		void RemoveAllMarkersInRange(const Vector3& position, float radius)
		{
			for (auto it = Databases::MarkerDb.begin(); it != Databases::MarkerDb.end();)
			{
				if (position.DistanceTo(it->m_position) < radius)
				{
					it = RemoveMarker(it);
				}
				else ++it;
			}
		}

		SpoonerMarker* CopyMarker(SpoonerMarker& marker)
		{
			SpoonerMarker newMarker = marker;
			Databases::MarkerDb.push_back(newMarker);
			return &Databases::MarkerDb.back();
		}
	}

}



