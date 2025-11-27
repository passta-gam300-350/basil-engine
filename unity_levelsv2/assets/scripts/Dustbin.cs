using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;

public class Dustbin : Behavior
{
    public float tossDistance = .9f;
    public int thrashRemaining = 3;
    private GameObject player;


    




    public void Init()
    {
        player = GameObject.Find("PlayerGroup");

    }
    public void Update()
    {
        float dist = Vector3.DistanceSqr(player.transform.position, transform.position);
        if (GameManager.instance.isHoldingThrash)
        {
            if (Input.GetKey(KeyCode.E))
            {
                Logger.Log(dist + "," + tossDistance * tossDistance);
                Logger.Log(dist < tossDistance * tossDistance);
                if (dist < tossDistance * tossDistance)
                {
                    Logger.Log("Tossing trash!");
                    GameManager.instance.ShowFree();
                    thrashRemaining--;


                    if (thrashRemaining == 0)
                    {
                        GameManager.instance.isCleaned = true;
                    }
                }
                Logger.Log("Destination: " + dist);
            }
           
        }

    }

    public void FixedUpdate()
    {

    }

}