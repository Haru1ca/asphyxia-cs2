#include "aim.h"

// used: sdk entity
#include "../../sdk/entity.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"
// used: cusercmd
#include "../../sdk/datatypes/usercmd.h"

// used: activation button
#include "../../utilities/inputsystem.h"

// used: cheat variables
#include "../../core/variables.h"

#include "../../sdk/interfaces/cgametracemanager.h"
#include "../../sdk/interfaces/ccsgoinput.h"

void F::LEGITBOT::AIM::OnMove(CUserCmd* pCmd, CBaseUserCmdPB* pBaseCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn)
{
	// Check if the legitbot is enabled
	if (!C_GET(bool, Vars.bLegitbot))
		return;
	if (!pLocalController->IsPawnAlive())
		return;
	AimAssist(pBaseCmd, pLocalPawn, pLocalController);
	if (!C_GET(bool, Vars.bSilentbot))
		return;
	SilentAim(pBaseCmd, pLocalPawn, pLocalController);
	if (!C_GET(bool, Vars.bTriggerbot))
		return;
	Triggerbot(pCmd, C_BaseEntity* localent, C_BaseEntity* playerent, C_CSPlayerPawn* local, C_CSPlayerPawn* player);
}

void  F::LEGIT::AIM::Triggerbot(CUserCmd* pUserCmd, C_CSPlayerPawn* pLocalPawn, CCSPlayerController* pLocalController)
{
	// Check if the activation key is down
	if (!IPT::IsKeyDown(C_GET(unsigned int, Vars.nTriggerbotActivationKey)))
		return;
    if (!pUserCmd)
        return;
    auto base = pUserCmd->m_csgoUserCmd.m_pBaseCmd;
    if (!base)
        return;
	if (!pLocalController->IsPawnAlive())
		return;
 	int  iIDEntIndex     = pLocalPawn->m_iIDEntIndex();
	if (iIDEntIndex == -1)
    	return;
    if ((localPlayerPawn->m_holdTargetIDTimer().m_timestamp() - localPlayerPawn->m_delayTargetIDTimer().m_timestamp() + 0.3) < C_GET(float, Vars.flTriggerbotDelay))
        return;
	C_BaseEntity* pEntity = I::GameResourceService->pGameEntitySystem->Get(iIDEntIndex);
	if (pEntity == nullptr)
		return;
	if (pEntity->GetTeam() == 0)
        return;
    if (pEntity->GetTeam() == pLocalController->GetTeam())
        return;
    cmd->m_nButtons.m_nValue |= IN_ATTACK;
}

QAngle_t GetRecoil(CBaseUserCmdPB* pCmd,C_CSPlayerPawn* pLocal)
{
	static QAngle_t OldPunch;//get last tick AimPunch angles
	if (pLocal->GetShotsFired() >= 1)//only update aimpunch while shooting
	{
		QAngle_t viewAngles = pCmd->pViewAngles->angValue;
		QAngle_t delta = viewAngles - (viewAngles + (OldPunch - (pLocal->GetAimPunchAngle() * 2.f)));//get current AimPunch angles delta

		return pLocal->GetAimPunchAngle() * 2.0f;//return correct aimpunch delta
	}
	else
	{
		return QAngle_t{ 0, 0 ,0};//return 0 if is not shooting
	}
}

QAngle_t GetAngularDifference(CBaseUserCmdPB* pCmd, Vector_t vecTarget, C_CSPlayerPawn* pLocal)
{
	// The current position
	Vector_t vecCurrent = pLocal->GetEyePosition();

	// The new angle
	QAngle_t vNewAngle = (vecTarget - vecCurrent).ToAngles();
	vNewAngle.Normalize(); // Normalise it so we don't jitter about

	// Store our current angles
	QAngle_t vCurAngle = pCmd->pViewAngles->angValue;

	// Find the difference between the two angles (later useful when adding smoothing)
	vNewAngle -= vCurAngle;

	return vNewAngle;
}

float GetAngularDistance(CBaseUserCmdPB* pCmd, Vector_t vecTarget, C_CSPlayerPawn* pLocal)
{
	return GetAngularDifference(pCmd, vecTarget, pLocal).Length2D();
}

void F::LEGITBOT::AIM::AimAssist(CBaseUserCmdPB* pUserCmd, C_CSPlayerPawn* pLocalPawn, CCSPlayerController* pLocalController)
{

	// Check if the activation key is down
	if (!IPT::IsKeyDown(C_GET(unsigned int, Vars.nLegitbotActivationKey)) && !C_GET(bool, Vars.bLegitbotAlwaysOn))
		return;

	// The current best distance
	float flDistance = INFINITY;
	// The target we have chosen
	CCSPlayerController* pTarget = nullptr;
	// Cache'd position
	Vector_t vecBestPosition = Vector_t();

	// Entity loop
	const int iHighestIndex = I::GameResourceService->pGameEntitySystem->GetHighestEntityIndex();

	auto aimPunch = GetRecoil(pUserCmd, pLocalPawn); //get AimPunch angles

	for (int nIndex = 1; nIndex <= iHighestIndex; nIndex++)
	{
		// Get the entity
		C_BaseEntity* pEntity = I::GameResourceService->pGameEntitySystem->Get(nIndex);
		if (pEntity == nullptr)
			continue;

		// Get the class info
		SchemaClassInfoData_t* pClassInfo = nullptr;
		pEntity->GetSchemaClassInfo(&pClassInfo);
		if (pClassInfo == nullptr)
			continue;

		// Get the hashed name
		const FNV1A_t uHashedName = FNV1A::Hash(pClassInfo->szName);

		// Make sure they're a player controller
		if (uHashedName != FNV1A::HashConst("CCSPlayerController"))
			continue;

		// Cast to player controller
		CCSPlayerController* pPlayer = reinterpret_cast<CCSPlayerController*>(pEntity);
		if (pPlayer == nullptr)
			continue;

		// Check the entity is not us
		if (pPlayer == pLocalController)
			continue;

		// Get the player pawn
		C_CSPlayerPawn* pPawn = I::GameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(pPlayer->GetPawnHandle());
		if (pPawn == nullptr)
			continue;

		// Make sure they're alive
		if (!pPlayer->IsPawnAlive())
			continue;

		// Check if they're an enemy
		if (!pLocalPawn->IsOtherEnemy(pPawn))
			continue;

		// Check if they're dormant
		CGameSceneNode* pCGameSceneNode = pPawn->GetGameSceneNode();
		if (pCGameSceneNode == nullptr || pCGameSceneNode->IsDormant())
			continue;

		// Get the position

		// Firstly, get the skeleton
		CSkeletonInstance* pSkeleton = pCGameSceneNode->GetSkeletonInstance();
		if (pSkeleton == nullptr)
			continue;
		// Now the bones
		Matrix2x4_t* pBoneCache = pSkeleton->pBoneCache;
		if (pBoneCache == nullptr)
			continue;

		const int iBone = 6; // You may wish to change this dynamically but for now let's target the head.

		// Get the bone's position
		Vector_t vecPos = pBoneCache->GetOrigin(iBone);

		// @note: this is a simple example of how to check if the player is visible

		// initialize trace, construct filterr and initialize ray
		GameTrace_t trace = GameTrace_t();
		TraceFilter_t filter = TraceFilter_t(0x1C3003, pLocalPawn, nullptr, 4);
		Ray_t ray = Ray_t();

		// cast a ray from local player eye positon -> player head bone
		// @note: would recommend checking for nullptrs
		I::GameTraceManager->TraceShape(&ray, pLocalPawn->GetEyePosition(), pPawn->GetGameSceneNode()->GetSkeletonInstance()->pBoneCache->GetOrigin(6), &filter, &trace);
		// check if the hit entity is the one we wanted to check and if the trace end point is visible
		if (trace.m_pHitEntity != pPawn || !trace.IsVisible())// if invisible, skip this entity
			continue;

		// Get the distance/weight of the move
		float flCurrentDistance = GetAngularDistance(pUserCmd, vecPos, pLocalPawn);
		if (flCurrentDistance > C_GET(float, Vars.flAimRange))// Skip if this move out of aim range
			continue;
		if (pTarget && flCurrentDistance > flDistance) // Override if this is the first move or if it is a better move
			continue;

		// Better move found, override.
		pTarget = pPlayer;
		flDistance = flCurrentDistance;
		vecBestPosition = vecPos;
	}

	// Check if a target was found
	if (pTarget == nullptr)
		return;

	// Point at them
	QAngle_t* pViewAngles = &(pUserCmd->pViewAngles->angValue); // Just for readability sake!

	// Find the change in angles
	QAngle_t vNewAngles = GetAngularDifference(pUserCmd, vecBestPosition, pLocalPawn);

	// Get the smoothing
	const float flSmoothing = C_GET(float, Vars.flSmoothing);

	// Apply smoothing and set angles
	pViewAngles->x += (vNewAngles.x - aimPunch.x ) / flSmoothing; // minus AimPunch angle to counteract recoil
	pViewAngles->y += (vNewAngles.y - aimPunch.y ) / flSmoothing;
	pViewAngles->Normalize();
}

void F::LEGITBOT::AIM::SilentAim(CBaseUserCmdPB* pUserCmd, C_CSPlayerPawn* pLocalPawn, CCSPlayerController* pLocalController)
{
	// Check if the activation key is down
	if (!IPT::IsKeyDown(C_GET(unsigned int, Vars.nLegitbotActivationKey)) && !C_GET(bool, Vars.bLegitbotAlwaysOn))
		return;

	// The current best distance
	float flDistance = INFINITY;
	// The target we have chosen
	CCSPlayerController* pTarget = nullptr;
	// Cache'd position
	Vector_t vecBestPosition = Vector_t();

	// Entity loop
	const int iHighestIndex = I::GameResourceService->pGameEntitySystem->GetHighestEntityIndex();

	auto aimPunch = GetRecoil(pUserCmd, pLocalPawn); //get AimPunch angles

	for (int nIndex = 1; nIndex <= iHighestIndex; nIndex++)
	{
		// Get the entity
		C_BaseEntity* pEntity = I::GameResourceService->pGameEntitySystem->Get(nIndex);
		if (pEntity == nullptr)
			continue;

		// Get the class info
		SchemaClassInfoData_t* pClassInfo = nullptr;
		pEntity->GetSchemaClassInfo(&pClassInfo);
		if (pClassInfo == nullptr)
			continue;

		// Get the hashed name
		const FNV1A_t uHashedName = FNV1A::Hash(pClassInfo->szName);

		// Make sure they're a player controller
		if (uHashedName != FNV1A::HashConst("CCSPlayerController"))
			continue;

		// Cast to player controller
		CCSPlayerController* pPlayer = reinterpret_cast<CCSPlayerController*>(pEntity);
		if (pPlayer == nullptr)
			continue;

		// Check the entity is not us
		if (pPlayer == pLocalController)
			continue;

		// Get the player pawn
		C_CSPlayerPawn* pPawn = I::GameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(pPlayer->GetPawnHandle());
		if (pPawn == nullptr)
			continue;

		// Make sure they're alive
		if (!pPlayer->IsPawnAlive())
			continue;

		// Check if they're an enemy
		if (!pLocalPawn->IsOtherEnemy(pPawn))
			continue;

		// Check if they're dormant
		CGameSceneNode* pCGameSceneNode = pPawn->GetGameSceneNode();
		if (pCGameSceneNode == nullptr || pCGameSceneNode->IsDormant())
			continue;

		// Get the position

		// Firstly, get the skeleton
		CSkeletonInstance* pSkeleton = pCGameSceneNode->GetSkeletonInstance();
		if (pSkeleton == nullptr)
			continue;
		// Now the bones
		Matrix2x4_t* pBoneCache = pSkeleton->pBoneCache;
		if (pBoneCache == nullptr)
			continue;

		const int iBone = 6; // You may wish to change this dynamically but for now let's target the head.

		// Get the bone's position
		Vector_t vecPos = pBoneCache->GetOrigin(iBone);

		// @note: this is a simple example of how to check if the player is visible

		// initialize trace, construct filterr and initialize ray
		GameTrace_t trace = GameTrace_t();
		TraceFilter_t filter = TraceFilter_t(0x1C3003, pLocalPawn, nullptr, 4);
		Ray_t ray = Ray_t();

		// cast a ray from local player eye positon -> player head bone
		// @note: would recommend checking for nullptrs
		I::GameTraceManager->TraceShape(&ray, pLocalPawn->GetEyePosition(), pPawn->GetGameSceneNode()->GetSkeletonInstance()->pBoneCache->GetOrigin(6), &filter, &trace);
		// check if the hit entity is the one we wanted to check and if the trace end point is visible
		if (trace.m_pHitEntity != pPawn || !trace.IsVisible()) // if invisible, skip this entity
			continue;

		// Get the distance/weight of the move
		float flCurrentDistance = GetAngularDistance(pUserCmd, vecPos, pLocalPawn);
		if (flCurrentDistance > C_GET(float, Vars.flSilentRange)) // Skip if this move out of aim range
			continue;
		if (pTarget && flCurrentDistance > flDistance) // Override if this is the first move or if it is a better move
			continue;

		// Better move found, override.
		pTarget = pPlayer;
		flDistance = flCurrentDistance;
		vecBestPosition = vecPos;
	}

	// Check if a target was found
	if (pTarget == nullptr)
		return;

	// Point at them
	QAngle_t pViewAngles = pUserCmd->pViewAngles->angValue; 

	// Find the change in angles
	QAngle_t vNewAngles = GetAngularDifference(pUserCmd, vecBestPosition, pLocalPawn);

	I::Input->GetUserCmd()->SetSubTickAngle({ pViewAngles.x + vNewAngles.x - aimPunch.x, pViewAngles.y + vNewAngles.y - aimPunch.y });
}
