package.path = package.path .. ";../data/scripts/?.lua"

-- transition functions
function idleToWalk()
    spaceButton = 32;
	if inputButtonWentDownThisFrame(spaceButton) == true then
		print("lets go walking!");
		return true;
	end
	return false;
end

function walkToIdle()
    spaceButton = 32;
	if inputButtonWentDownThisFrame(spaceButton) == true then
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
				idle = {{targetState="walk", transitionKey="l_foot_down", delayInSeconds = 1, logic = idleToWalk }},
				-- walk transitions
				walk = {{targetState="idle", transitionKey="l_foot_down", delayInSeconds = 1, logic = walkToIdle }},
			  },
}

function evaluate(currentState)
    print(currentState)
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
			-- need to push the transition
			return transition.targetState, sourceAnim,targetAnim,transitionKey;
		end
	end
	return currentState,nil,nil,nil;
end

-- returns the starting animation and state we wish to use
function start()
	startState = "idle";
	stateMap = stateMachine.states[startState];
	return startState ,stateMap.animation;
end
