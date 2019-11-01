package.path = package.path .. ";../data/scripts/?.lua"

-- transition functions
function idleToWalk()
    spaceButton = 32;
    upArrow = 38;
	if inputButtonWentDownThisFrame(upArrow) == true then
		print("lets go walking!");
		return true;
	end
	return false;
end

function walkToIdle()
    spaceButton = 32;
    upArrow = 38;
	if inputButtonDown(upArrow) == false then
		print("lets go idle");
		return true;
	end
	return false;
end


local stateMachine = {

-- states
states = {
	idle = { animation= "knightBIdle"},
	walk = { animation= "knightBWalk"}
        },
-- transitions
transitions = {
				-- idle transitions
				idle = {{targetState="walk", transitionKey="l_foot_down", 
				         transitionLenInSeconds=0.3, logic=idleToWalk,
						 cogSpeed = 0.0115}},
				-- walk transitions
				walk = {{targetState="idle", transitionKey="l_foot_down", 
						 transitionLenInSeconds=0.3, logic=walkToIdle,
						 cogSpeed=0.0}},
			  },
}

function evaluate(currentState)
	-- lets get the current state
	stateMap = stateMachine.states[currentState];
	
	-- lets get the transitions and start to iterate over them
	transitions = stateMachine.transitions[currentState];
	for state,transition in pairs(transitions) do
		result = transition.logic(); 
		if result == true then
			-- the transition was taken!! we need to perform the transition
			targetState = stateMachine.states[transition.targetState]; 
			targetAnim = targetState.animation;
			sourceAnim = stateMap.animation; 
			transitionKey = transition.transitionKey;
			transitionLen = transition.transitionLenInSeconds;
			speed = transition.cogSpeed;
			-- need to push the transition
			return transition.targetState, sourceAnim,targetAnim,transitionKey, transitionLen,speed;
		end
	end
	return currentState,nil,nil,nil,nil,nil;
end

-- returns the starting animation and state we wish to use
function start()
	startState = "idle";
	stateMap = stateMachine.states[startState];
	return startState ,stateMap.animation, 0.0;
end
