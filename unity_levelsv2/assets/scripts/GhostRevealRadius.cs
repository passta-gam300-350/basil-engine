using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;
using System;



public class GhostRevealRadius : Behavior
{
    public float revealRadius = 1.5f;
    public float fakeGhostDuration = 0.5f;

    private GameObject fakeGhost;
    private Audio shockVO;
    private GameObject realGhost;
    private GhostBehavior ghostBehavior;

    private GameObject player;

    private bool triggered = false;
    private bool waitingForSwap = false;

    private float timer = 0f;


    public void Init()
    {
        player = GameObject.Find("PlayerGroup");

        fakeGhost = GameObject.Find("GhostPreview");
        realGhost = GameObject.Find("Ghost");

        GameObject shockObj = GameObject.Find("GhostShockVO");

        if (shockObj != null)
        {
            shockVO = shockObj.transform.GetComponent<Audio>();

            if (shockVO == null)
                Logger.Warn("GhostRevealRadius: Shock VO found but Audio missing!");
        }
        else
        {
            Logger.Warn("GhostRevealRadius: GhostShockVO not found!");
        }

        if (realGhost != null)
            ghostBehavior = realGhost.transform.GetComponent<GhostBehavior>();
        else
            Logger.Warn("GhostRevealRadius: RealGhost not found!");

        if (fakeGhost != null)
            fakeGhost.visibility = false;
        else
            Logger.Warn("GhostRevealRadius: FakeGhost not found!");
    }

    public void Update()
    {
        if (player == null)
            return;

        // Already triggered -> count down
        if (waitingForSwap)
        {
            timer += Time.deltaTime;

            if (timer >= fakeGhostDuration)
            {
                FinishReveal();
            }

            return;
        }

        if (triggered)
            return;

        float distSqr = Vector3.DistanceSqr(
            transform.position,
            player.transform.position
        );

        float radiusSqr = revealRadius * revealRadius;

        if (distSqr <= radiusSqr)
        {
            TriggerReveal();
        }
    }

    private void TriggerReveal()
    {
        triggered = true;
        waitingForSwap = true;
        timer = 0f;

        Logger.Log("Ghost reveal triggered");

        if (fakeGhost != null)
            fakeGhost.visibility = true;

        if (shockVO != null)
            shockVO.Play();
    }

    private void FinishReveal()
    {
        waitingForSwap = false;

        if (fakeGhost != null)
            GameObject.Destroy(fakeGhost);

        if (ghostBehavior != null)
            ghostBehavior.ActivateGhost();

        GameObject.Destroy(gameObject);
    }
}