using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;

public class TrashCollectable : Behavior
{
    private GameObject player;
    public float collect_distance = 0.4f;
    



    public void Init()
    {

    }

    public void Update()
    {
        if (player == null)
        {
            player =  GameObject.Find("PlayerGroup");
            return;
        } 

        float distance = Vector3.DistanceSqr(transform.position, player.transform.position);
        if (distance <= collect_distance && Input.GetKey(KeyCode.E))
        {
            if (GameManager.instance.isHoldingThrash)
            {
                Logger.Log("No hands to use!");
                return;
            }

            //TODO: REMOVAL?
            Logger.Log("Player should collect the trash!");
            gameObject.transform.position = new Vector3(1000, 1000, 1000);
            GameObject.Destroy(gameObject);

            GameManager.instance.ShowCollected();
            
        }
        //Logger.Log("Distance" + distance);
    }

    public void FixedUpdate()
    {
    }
}
