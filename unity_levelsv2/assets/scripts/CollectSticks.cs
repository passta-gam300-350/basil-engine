using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;

public class CollectSticks : Behavior
{
    private GameObject player;
    public float collect_distance = 0.4f;




    public void Init()
    {
        player = GameObject.Find("PlayerGroup");
    }

    public void Update()
    {


        float distance = Vector3.DistanceSqr(transform.position, player.transform.position);
        if (distance <= collect_distance && Input.GetKey(KeyCode.E))
        {
            if (GameManager.instance.isHoldingThrash)
            {
                Logger.Log("No hands to use!");
                return;
            }

            //TODO: REMOVAL?
            Logger.Log("Player should collect the sticks!");
            gameObject.transform.position = new Vector3(1000, 1000, 1000);
            GameObject.Destroy(gameObject);

            PuzzleManager.manager.UnlockSticks();



        }
        //Logger.Log("Distance" + distance);
    }

    public void FixedUpdate()
    {
    }
}