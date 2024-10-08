#pragma once
// used: [ext] minhook
// @credits: https://github.com/TsudaKageyu/minhook
#include "../../dependencies/minhook/minhook.h"
#include "../../dependencies/Vook.h"
// used: l_print
#include "log.h"

template <typename T>
class CBaseHookObject
{
public:
	bool Vreate(void* pFunction, void* pDetour)
	{
		if (pFunction == nullptr || pDetour == nullptr)
			return false;

		pBaseFn = pFunction;
		pReplaceFn = pDetour;
		if (!Vook ::Hook(
			pBaseFn, // The initial function's address
			pReplaceFn, // The hook's address
			&pOriginalFn // The original's address
			))
		//if (const MH_STATUS status = MH_CreateHook(pBaseFn, pReplaceFn, &pOriginalFn); status != MH_OK)
		{
			/*
#ifdef _DEBUG
			L_PRINT(LOG_ERROR) << CS_XOR("failed to create hook, status: \"") << MH_StatusToString(status) << CS_XOR("\" with base address: ") << L::AddFlags(LOG_MODE_INT_SHOWBASE | LOG_MODE_INT_FORMAT_HEX) << reinterpret_cast<std::uintptr_t>(pBaseFn);
#else
			L_PRINT(LOG_ERROR) << CS_XOR("failed to create hook");
#endif
*/
			L_PRINT(LOG_ERROR) << CS_XOR("failed to create hook");
			CS_ASSERT(false);
			return false;
		}

		//if (!Replace())
		//return false;

		return true;
	}
	/// setup hook and replace function
	/// @returns: true if hook has been successfully created, false otherwise
	bool Create(void* pFunction, void* pDetour)
	{
		if (pFunction == nullptr || pDetour == nullptr)
			return false;

		pBaseFn = pFunction;
		pReplaceFn = pDetour;

		if (const MH_STATUS status = MH_CreateHook(pBaseFn, pReplaceFn, &pOriginalFn); status != MH_OK)
		{

#ifdef _DEBUG
			L_PRINT(LOG_ERROR) << CS_XOR("failed to create hook, status: \"") << MH_StatusToString(status) << CS_XOR("\" with base address: ") << L::AddFlags(LOG_MODE_INT_SHOWBASE | LOG_MODE_INT_FORMAT_HEX) << reinterpret_cast<std::uintptr_t>(pBaseFn);
#else
			L_PRINT(LOG_ERROR) << CS_XOR("failed to create hook");
#endif
			CS_ASSERT(false);
			return false;
		}

		if (!Replace())
			return false;

		return true;
	}

	/// patch memory to jump to our function instead of original
	/// @returns: true if hook has been successfully applied, false otherwise
	bool Replace()
	{
		// check is hook has been created
		if (pBaseFn == nullptr)
			return false;

		// check that function isn't already hooked
		if (bIsHooked)
			return false;

		if (const MH_STATUS status = MH_EnableHook(pBaseFn); status != MH_OK)
		{
#ifdef _DEBUG
			L_PRINT(LOG_ERROR) << CS_XOR("failed to enable hook, status: \"") << MH_StatusToString(status) << CS_XOR("\" with base address: ") << L::AddFlags(LOG_MODE_INT_SHOWBASE | LOG_MODE_INT_FORMAT_HEX) << reinterpret_cast<std::uintptr_t>(pBaseFn);
#else
			L_PRINT(LOG_ERROR) << CS_XOR("failed to enable hook");
#endif

			CS_ASSERT(false);
			return false;
		}

		// switch hook state
		bIsHooked = true;
		return true;
	}

		bool Vemove()
	{
		// restore it at first
		//if (!Restore())
		//return false;
		if (!Vook::Unhook(
			pBaseFn // The initial function's address
			))
		//if (const MH_STATUS status = MH_RemoveHook(pBaseFn); status != MH_OK)
		{
			/*
#ifdef _DEBUG
			L_PRINT(LOG_ERROR) << CS_XOR("failed to remove hook, status: \"") << MH_StatusToString(status) << CS_XOR("\" with base address: ") << L::AddFlags(LOG_MODE_INT_SHOWBASE | LOG_MODE_INT_FORMAT_HEX) << reinterpret_cast<std::uintptr_t>(pBaseFn);
#else
			L_PRINT(LOG_ERROR) << CS_XOR("failed to remove hook");
#endif
*/
			L_PRINT(LOG_ERROR) << CS_XOR("failed to remove hook");
			CS_ASSERT(false);
			return false;
		}

		return true;
	}

	/// restore original function call and cleanup hook data
	/// @returns: true if hook has been successfully removed, false otherwise
	bool Remove()
	{
		// restore it at first
		if (!Restore())
			return false;
		if (const MH_STATUS status = MH_RemoveHook(pBaseFn); status != MH_OK)
		{

#ifdef _DEBUG
			L_PRINT(LOG_ERROR) << CS_XOR("failed to remove hook, status: \"") << MH_StatusToString(status) << CS_XOR("\" with base address: ") << L::AddFlags(LOG_MODE_INT_SHOWBASE | LOG_MODE_INT_FORMAT_HEX) << reinterpret_cast<std::uintptr_t>(pBaseFn);
#else
			L_PRINT(LOG_ERROR) << CS_XOR("failed to remove hook");
#endif
			CS_ASSERT(false);
			return false;
		}

		return true;
	}

	/// restore patched memory to original function call
	/// @returns: true if hook has been successfully restored, false otherwise
	bool Restore()
	{
		// check that function is hooked
		if (!bIsHooked)
			return false;

		if (const MH_STATUS status = MH_DisableHook(pBaseFn); status != MH_OK)
		{
#ifdef _DEBUG
			L_PRINT(LOG_ERROR) << CS_XOR("failed to restore hook, status: \"") << MH_StatusToString(status) << CS_XOR("\" with base address: ") << L::AddFlags(LOG_MODE_INT_SHOWBASE | LOG_MODE_INT_FORMAT_HEX) << reinterpret_cast<std::uintptr_t>(pBaseFn);
#else
			L_PRINT(LOG_ERROR) << CS_XOR("failed to restore hook");
#endif

			CS_ASSERT(false);
			return false;
		}

		// switch hook state
		bIsHooked = false;
		return true;
	}

	/// @returns: original, unwrapped function that would be called without the hook
	CS_INLINE T GetOriginal()
	{
		return reinterpret_cast<T>(pOriginalFn);
	}

	/// @returns: true if hook is applied at the time, false otherwise
	CS_INLINE bool IsHooked() const
	{
		return bIsHooked;
	}

private:
	// current hook state
	bool bIsHooked = false;
	// function base handle
	void* pBaseFn = nullptr;
	// function that being replace the original call
	void* pReplaceFn = nullptr;
	// original function
	void* pOriginalFn = nullptr;
};
