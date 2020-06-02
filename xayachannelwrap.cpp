#define XAYAWRAP_API __declspec(dllexport) __stdcall

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <sstream>
#include <csetjmp>
#include <csignal>

#include "gamechannel/wrapper.hpp"


#include <json/json.h>
#include <jsonrpccpp/client.h>

#include <glog/logging.h>


typedef bool (*ISSTATEVALID_CALLBACK)(const char*);
typedef bool (*ISSTATESEQUAL_CALLBACK)(const char*, const char*);
typedef int (*WHOSETURN_CALLBACK)(const char*, const char*);
typedef unsigned (*TURNCOUNT_CALLBACK)(const char*, const char*);
typedef bool (*APPLYMOVE_CALLBACK)(const char*, const char*, const char*, const char*, const char**);
typedef const char* (*RESOLUTIONMOVE_CALLBACK)(const char*, const char*);
typedef const char* (*DISPUTEMOVE_CALLBACK)(const char*, const char*);
typedef const char* (*MAYBEAUTOMOVE_CALLBACK)(const char*, const char*, const char*, const char*, const char*, const char**);
typedef const char* (*MAYBEONCHAINMOVE_CALLBACK)(const char*, const char*, const char*, const char*, const char*, const char**);

static Json::StreamWriterBuilder jsonWriterBuilder;

static ISSTATEVALID_CALLBACK isStateValidCallback;
static ISSTATESEQUAL_CALLBACK isStatesEqualCallback;
static WHOSETURN_CALLBACK whoseTurnCallback;
static TURNCOUNT_CALLBACK turnCountCallback;
static APPLYMOVE_CALLBACK applyMoveCallback;
static RESOLUTIONMOVE_CALLBACK resolutionMoveCallback;
static DISPUTEMOVE_CALLBACK disputeMoveCallback;
static MAYBEAUTOMOVE_CALLBACK maybeAutoMoveCallback;
static MAYBEONCHAINMOVE_CALLBACK maybeOnChainMoveCallback;

/* This is needed to workaround Unity crash
on abort signal;*/
jmp_buf env;

void
on_sigabrt(int signum)
{
  longjmp(env, 1);
}


bool
IsStateValidExtern(const xaya::BoardState& state)
{
return isStateValidCallback(state.c_str());
}

bool
IsStatesEqualExtern(const xaya::BoardState& a, const xaya::BoardState& b)
{
return isStatesEqualCallback(a.c_str(),b.c_str() );
}

int
WhoseTurnCallbackExtern(const std::string& metadata, const xaya::BoardState& state)
{
return whoseTurnCallback(metadata.c_str(), state.c_str());
}

unsigned
TurnCountExtern(const std::string& metadata, const xaya::BoardState& state)
{
return turnCountCallback(metadata.c_str(), state.c_str());
}

bool
ApplyMoveCallbackExtern(const std::string& channelId, const std::string& metadata,
                   const xaya::BoardState& state, const xaya::BoardMove& mv,
                   xaya::BoardState& newState)
{
	
const char* newStateFill;
	
bool result = applyMoveCallback(channelId.c_str(), metadata.c_str(), state.c_str(), mv.c_str(), &newStateFill);

(&newState)->assign(newStateFill);    

return result;
}

Json::Value
ResolutionMoveExtern(const std::string& channelId, const std::string& proof)
{
	Json::Value result = resolutionMoveCallback(channelId.c_str(), proof.c_str());
	//const std::string resultSTR = Json::writeString(jsonWriterBuilder, result);
	return result;
}

Json::Value
DisputeMoveExtern(const std::string& channelId, const std::string& proof)
{
	Json::Value result = disputeMoveCallback(channelId.c_str(), proof.c_str());
	//const std::string resultSTR = Json::writeString(jsonWriterBuilder, result);
	return result;
}

bool
MaybeAutoMoveExtern(const std::string& channelId, const std::string& meta,
                         const std::string& playerName,
                         const xaya::BoardState& state, const xaya::PrivateState& priv,
                         xaya::BoardMove& mv)
{
	const char* mvFill;
	
	bool result = maybeAutoMoveCallback(channelId.c_str(), meta.c_str(), playerName.c_str(),
	state.c_str(), priv.c_str(), &mvFill);
	
    (&mv)->assign(mvFill); 	
	
	return result;
}

bool
MaybeOnChainMoveExtern(const std::string& channelId,
                            const std::string& meta,
                            const std::string& playerName,
                            const xaya::BoardState& state, const xaya::PrivateState& priv,
                            Json::Value& mv)
{
	

    const char* mvFill;
    std::string resultSTR = Json::writeString(jsonWriterBuilder, mv);

	bool result =  maybeAutoMoveCallback(channelId.c_str(), meta.c_str(), playerName.c_str(),
	state.c_str(), priv.c_str(), &mvFill);
	
	 (&resultSTR)->assign(mvFill); 
	
	return result;
}


int
RunCallbackChannelFromExternal (std::string gameId, std::string channelId, std::string playerName,
                   std::string XayaRpcUrl, std::string GspRpcUrl, std::string BroadcastRpcUrl, 
				   int ChannelRpcPort, 
				   std::string glogName, std::string glogDataDir)
{
  FLAGS_alsologtostderr = 1;
  FLAGS_log_dir = glogDataDir;

  google::InitGoogleLogging(glogName.c_str());

  jsonWriterBuilder["commentStyle"] = "None";
  jsonWriterBuilder["enableYAMLCompatibility"] = false;
  jsonWriterBuilder["indentation"] = "";

  xaya::BoardRulesCallbacks brcb;
  brcb.IsStateValid = IsStateValidExtern;
  brcb.ApplyMove = ApplyMoveCallbackExtern;
  brcb.StatesEqual = IsStatesEqualExtern;
  brcb.WhoseTurn = WhoseTurnCallbackExtern;
  brcb.TurnCount = TurnCountExtern;
  
  xaya::OpenChannelCallbacks opcb;
  opcb.ResolutionMove = ResolutionMoveExtern;
  opcb.DisputeMove = DisputeMoveExtern;
  opcb.MaybeAutoMove = MaybeAutoMoveExtern;
  opcb.MaybeOnChainMove = MaybeOnChainMoveExtern;
  
  xaya::CallbackChannelConfig config; 
  
  config.RuleCallbacks = brcb;
  config.ChannelCallbacks = opcb;
  config.GameId = gameId;
  config.ChannelId = channelId;
  config.PlayerName = playerName;
  config.XayaRpcUrl = XayaRpcUrl;
  config.GspRpcUrl = GspRpcUrl;
  config.BroadcastRpcUrl = BroadcastRpcUrl;
  config.ChannelRpcPort = ChannelRpcPort;
  
  LOG(WARNING) << "GameId: " << gameId;
  LOG(WARNING) << "ChannelId: " << channelId;
  LOG(WARNING) << "PlayerName: " << playerName;
  LOG(WARNING) << "XayaRpcUrl: " << XayaRpcUrl;
  LOG(WARNING) << "GspRpcUrl: " << GspRpcUrl;
  LOG(WARNING) << "BroadcastRpcUrl: " << BroadcastRpcUrl;
  LOG(WARNING) << "ChannelRpcPort: " << ChannelRpcPort;
  LOG(WARNING) << "glogName: " << glogName;
  LOG(WARNING) << "glogDataDir: " << glogDataDir;  
  
   if (setjmp(env) == 0) {
    signal(SIGABRT, &on_sigabrt);
  xaya::RunCallbackChannel(config);
    } else {
  }
  
  google::ShutdownGoogleLogging();
  
  return 0;
}

extern "C" 
{
	
XAYAWRAP_API void
setApplyMoveCallback(APPLYMOVE_CALLBACK callback)
{
  if (callback) {
    applyMoveCallback = callback;
  }
}	
	
XAYAWRAP_API void
setMaybeOnChainMoveCallback(MAYBEONCHAINMOVE_CALLBACK callback)
{
  if (callback) {
    maybeOnChainMoveCallback = callback;
  }
}		
	
XAYAWRAP_API void
setMaybeAutoMoveCallback(MAYBEAUTOMOVE_CALLBACK callback)
{
  if (callback) {
    maybeAutoMoveCallback = callback;
  }
}		
	
XAYAWRAP_API void
setDisputeMoveCallback(DISPUTEMOVE_CALLBACK callback)
{
  if (callback) {
    disputeMoveCallback = callback;
  }
}	
	
XAYAWRAP_API void
setResolutionMoveCallback(RESOLUTIONMOVE_CALLBACK callback)
{
  if (callback) {
    resolutionMoveCallback = callback;
  }
}	
	
XAYAWRAP_API void
setTurnCountCallback(TURNCOUNT_CALLBACK callback)
{
  if (callback) {
    turnCountCallback = callback;
  }
}		
	
XAYAWRAP_API void
setWhoseTurnCallback(WHOSETURN_CALLBACK callback)
{
  if (callback) {
    whoseTurnCallback = callback;
  }
}		
	
XAYAWRAP_API void
setIsStatesEqualCallback(ISSTATESEQUAL_CALLBACK callback)
{
  if (callback) {
    isStatesEqualCallback = callback;
  }
}		
	
XAYAWRAP_API void
setIsStateValidCallback(ISSTATEVALID_CALLBACK callback)
{
  if (callback) {
    isStateValidCallback = callback;
  }
}	

XAYAWRAP_API int
CSharp_RunCallbackChannelFromExternal(char* jarg1, char* jarg2, char* jarg3, char* jarg4, char* jarg5, char* jarg6, int jarg7, char* jarg8, char* jarg9)
{
  int jresult;
  int result;

  std::string arg1;
  std::string arg2;
  std::string arg3;
  std::string arg4;
  std::string arg5;
  std::string arg6;
  int arg7;
  std::string arg8;
  std::string arg9;

  (&arg1)->assign(jarg1);
  (&arg2)->assign(jarg2);
  (&arg3)->assign(jarg3);
  (&arg4)->assign(jarg4);
  (&arg5)->assign(jarg5);
  (&arg6)->assign(jarg6);
  
  arg7 = (int)jarg7;
  
  (&arg8)->assign(jarg8);
  (&arg9)->assign(jarg9);

  result = (int)RunCallbackChannelFromExternal(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);

  jresult = result;
  return jresult;
}

}
