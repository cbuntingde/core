BINDING_HEADER_VBOTS = "vbots";

BINDING_NAME_VBOTS_TOGGLE = "Show/Hide Bot Panel";
BINDING_NAME_VBOTS_CLONE = "Clone bots";
BINDING_NAME_VBOTS_REMOVE = "Remove bots";

SLASH_VBOTS1 = "/vbots";
SlashCmdList["VBOTS"] = function() ShowToggle(); end

local CMD_CLONE = ".partybot clone";
local CMD_REMOVE = ".partybot remove";
local CMD_DIE = ".die";
local CMD_REVIVE = ".revive";
local CMD_PARTYBOT_ADD = ".partybot add ";
local CMD_PARTYBOT_SETROLE = ".partybot setrole ";
local CMD_PARTYBOT_CCMARK = ".partybot ccmark ";
local CMD_PARTYBOT_FOCUSMARK = ".partybot focusmark ";
local CMD_PARTYBOT_FOLLOW = ".partybot follow ";
local CMD_PARTYBOT_HOLD = ".partybot hold";
local CMD_PARTYBOT_STAY = ".partybot stay";
local CMD_PARTYBOT_ASSIST = ".partybot assist";
local CMD_PARTYBOT_SKILL = ".partybot skill ";
local CMD_BATTLEBOT_ADD = ".battlebot add ";

local SHOW = 0;

function OpenFrame()
    if SHOW == 0 then
        vbotsFrame:Show();
        SHOW = 1;
    end
end

function ShowToggle()
    if SHOW == 1 then
        vbotsFrame:Hide();
        SHOW = 0;
    else
        vbotsFrame:Show();
        SHOW = 1;
    end
end

function CloseFrame()
    vbotsFrame:Hide();
    SHOW = 0;
end

function vbotsButtonFrame_OnClick()
    if SHOW == 1 then
        vbotsFrame:Hide();
        SHOW = 0;
    else
        vbotsFrame:Show();
        SHOW = 1;
    end
end

function SubPartyBotClone(self)
    local funcname = self:GetText();
    if funcname == "Clone" then
        SendChatMessage(CMD_CLONE);
    else
        SendChatMessage(CMD_REMOVE);
    end
end

function SubPartyBotDie()
    SendChatMessage(CMD_DIE);
end

function SubPartyBotRevive()
    SendChatMessage(CMD_REVIVE);
end

function SubPartyBotAdd(self, arg1)
    local PlayerName = UnitName("player");
    local ranbotlevel = UnitLevel("player");
    SendChatMessage(CMD_PARTYBOT_ADD .. arg1 .. " " .. ranbotlevel);
end

function SubPartyBotSetRole(arg1)
    SendChatMessage(CMD_PARTYBOT_SETROLE .. arg1);
end

function SubPartyBotSetCCMark(arg1)
    SendChatMessage(CMD_PARTYBOT_CCMARK .. arg1);
end

function SubPartyBotFollow(arg1)
    SendChatMessage(CMD_PARTYBOT_FOLLOW .. arg1);
end

function SubPartyBotHold()
    SendChatMessage(CMD_PARTYBOT_HOLD);
end

function SubPartyBotStay()
    SendChatMessage(CMD_PARTYBOT_STAY);
end

function SubPartyBotAssist()
    SendChatMessage(CMD_PARTYBOT_ASSIST);
end

function SubPartyBotSkill(arg1)
    SendChatMessage(CMD_PARTYBOT_SKILL .. arg1);
end

function SubPartyBotSetFocusMark(arg1)
    SendChatMessage(CMD_PARTYBOT_FOCUSMARK .. arg1);
end

function Brackets()
    local ranbotlevel;
    local PlayerLevel = UnitLevel("player");
    if PlayerLevel <= 60 and PlayerLevel >= 50 then
        ranbotlevel = math.random(50, 60);
    elseif PlayerLevel >= 40 and PlayerLevel <= 49 then
        ranbotlevel = math.random(40, 49);
    elseif PlayerLevel >= 30 and PlayerLevel <= 39 then
        ranbotlevel = math.random(30, 39);
    elseif PlayerLevel >= 20 and PlayerLevel <= 29 then
        ranbotlevel = math.random(20, 29);
    elseif PlayerLevel >= 10 and PlayerLevel <= 19 then
        ranbotlevel = math.random(10, 19);
    else
        ranbotlevel = math.random(10, 19);
    end
    return ranbotlevel;
end

function SubBattleBotAdd(self, arg1, arg2)
    local ranbotnumber = Brackets();
    SendChatMessage(CMD_BATTLEBOT_ADD .. arg1 .. " " .. arg2 .. " " .. ranbotnumber);
end

function vbotsFrame_OnLoad()
    vbotsFrame:RegisterForDrag("LeftButton");
    vbotsFrame:SetBackdropColor(0, 0, 0, 0.8);
    HideUIPanel(vbotsFrame);
    tinsert(UISpecialFrames, "vbotsFrame");
end

function vbotsFrame_OnDragStart(arg1)
    if arg1 == "LeftButton" then
        vbotsFrame:StartMoving();
    end
end

function vbotsFrame_OnDragStop()
    vbotsFrame:StopMovingOrSizing();
end

function vbotsFrame_OnShow()
    PlaySound("igMainMenuOpen");
end

function vbotsFrame_OnHide()
    PlaySound("igMainMenuClose");
end
