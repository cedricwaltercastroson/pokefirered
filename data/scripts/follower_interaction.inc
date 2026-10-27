@ The follower is a dynamically spawned object; its script must be reached
@ by the early local-ID check in GetInteractedObjectEventScript.
EventScript_FollowerTalk::
	lock
	@ Follower_PrepareInteraction handles facing once lock finishes.
	special Follower_PrepareInteraction
	@ Only a deliberate follower interaction plays its species-specific cry.
	special Follower_PlayInteractionCry
	msgbox gStringVar4
	waitmoncry
	special Follower_EndInteraction
	release
	end
