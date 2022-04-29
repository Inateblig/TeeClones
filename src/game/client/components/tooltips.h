#ifndef GAME_CLIENT_COMPONENTS_TOOLTIPS_H
#define GAME_CLIENT_COMPONENTS_TOOLTIPS_H

#include <cstdint>
#include <game/client/component.h>
#include <game/client/ui.h>

#include <unordered_map>

struct CTooltip
{
	CUIRect m_Rect;
	const char *m_pText;
	float m_WidthHint;
};

/**
 * A component that manages and renders UI tooltips.
 * 
 * Should be among the last components to render.
 */
class CTooltips : public CComponent
{
	std::unordered_map<uintptr_t, CTooltip> m_Tooltips;
	CTooltip *m_pActiveTooltip;
	int64_t HoverTime;

	/**
     * The passed tooltip is only actually set if there is no currently active tooltip.
     * 
     * @param Tooltip A reference to the tooltip that should be active.
     */
	void SetActiveTooltip(CTooltip &Tooltip);

	inline void ClearActiveTooltip();

public:
	CTooltips();
	virtual int Sizeof() const override { return sizeof(*this); }

	/**
	 * Adds the tooltip to a cache and renders it when active.
     * 
     * On the first call to this function, the data passed is cached, afterwards the calls are used to detect if the tooltip should be activated.
     * 
	 * For now only works correctly with single line tooltips, since Text width calculation gets broken when there are multiple lines.
	 *
	 * @param pID The ID of the tooltip. Usually a reference to some g_Config value.
	 * @param pNearTo Place the tooltip near this rect.
	 * @param pText The text to display in the tooltip
	 */
	void DoToolTip(const void *pID, const CUIRect *pNearRect, const char *pText, float WidthHint = -1.0f);

	virtual void OnReset() override;
	virtual void OnRender() override;
};

#endif
