using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;

public class CollectFlap : Behavior
{
    private GameObject player;
    private PlayerController3D controller;
    public float collect_distance = 0.4f;

    public void Init()
    {
        player = GameObject.Find("PlayerGroup");

        if (player != null)
        {
            controller = player.transform.GetComponent<PlayerController3D>();
        }

        if (controller == null)
        {
            Logger.Warn("CollectSticks: PlayerController3D not found!");
        }
    }

    public void Update()
    {
        if (player == null || controller == null)
            return;

        float distance = Vector3.DistanceSqr(transform.position, player.transform.position);
        if (distance <= collect_distance && controller.wantsToCollect)
        {
            if (TrashBag.instance.trashInHand >= TrashBag.instance.maxTrashInHand)
            {
                Logger.Log("No hands to use!");
                return;
            }

            Logger.Log("Flap collected!");

            GameObject.Destroy(gameObject);
            PuzzleManager.manager.UnlockFlap();

        }
        //Logger.Log("Distance" + distance);
    }

    public void FixedUpdate()
    {
    }
}