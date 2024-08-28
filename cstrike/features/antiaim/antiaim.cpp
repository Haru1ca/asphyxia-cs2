#include "antiaim.hpp"
#include <ctime>
#include <cmath>
#include "../../core/variables.h"

void F::ANTIAIM::RunAA(CUserCmd* pCmd)
{
	if (!C_GET(bool, Vars.bAntiAim))
		return;

	if (!I::Engine->IsConnected() || !I::Engine->IsInGame()) // Checking if you are connected and in game
		return;

	if (!SDK::LocalController || !SDK::LocalController->IsPawnAlive()) // Checking if your spectating and alive
		return;

	if (pCmd->nButtons.nValue & ECommandButtons::IN_USE || pCmd->nButtons.nValue & ECommandButtons::IN_ATTACK) // Checking if you are not pressing e or attacking
		return;

	if (int32_t nMoveType = SDK::LocalController->GetMoveType(); nMoveType == MOVETYPE_NOCLIP || nMoveType == MOVETYPE_LADDER)
		return;

	C_CSPlayerPawn* pLocalPawn = I::GameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(SDK::LocalController->GetPawnHandle());

	if (pLocalPawn == nullptr)
		return;

	if (SDK::LocalController->IsThrowingGrenade(SDK::LocalController->GetPlayerWeapon(SDK::LocalPawn)))
		return;

	float flPitch = MATH::fnRandomFloat(-59.F,59.F);
	float flYaw = pCmd->csgoUserCmd.pBaseCmd->nClientTick % 180;

	pCmd->csgoUserCmd.pBaseCmd->pViewangles->angValue.x = flPitch;
	pCmd->csgoUserCmd.pBaseCmd->pViewangles->angValue.y += flYaw;

	pCmd->csgoUserCmd.pBaseCmd->pViewangles->angValue.y = MATH::AngleNormalize(pCmd->csgoUserCmd.pBaseCmd->pViewangles->angValue.y);
	pCmd->csgoUserCmd.pBaseCmd->pViewangles->angValue.x = MATH::AngleNormalize(pCmd->csgoUserCmd.pBaseCmd->pViewangles->angValue.x);
	pCmd->csgoUserCmd.pBaseCmd->pViewangles->angValue.z = 0.f;
}