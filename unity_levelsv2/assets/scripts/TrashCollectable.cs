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
            player =  GameObject.Find("Player");
            return;
        } 

        float distance = Vector3.DistanceSqr(transform.position, player.transform.position);
        if (distance <= collect_distance && Input.GetKey(KeyCode.E))
        {
            //TODO: REMOVAL?
            Logger.Log("Player should collect the trash!");
            GameObject.Destroy(gameObject);
        }
        Logger.Log("Distance" + distance);
    }

    public void FixedUpdate()
    {
    }
}
