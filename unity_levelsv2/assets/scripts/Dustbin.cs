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
    private GameObject[] garbage;
    private System.Random random = new System.Random();

    




    public void Init()
    {
        player = GameObject.Find("PlayerGroup");
        garbage = new GameObject[3];
        for (int i = 0; i < garbage.Length; i++)
        {
            string objectName = "garbage_bin_" + (i + 1);
            garbage[i] = GameObject.Find(objectName);
            if (garbage[i] == null)
            {
                Logger.Warn("Cannot find game object: " + objectName);
            }
        }
    }
    public void Update()
    {
        if (player == null)
            return;

        float dist = Vector3.DistanceSqr(player.transform.position, transform.position);
        if (GameManager.instance.trashInHand >= GameManager.instance.maxTrashInHand)
        {
            if (Input.GetKey(KeyCode.E))
            {
                Logger.Log(dist + "," + tossDistance * tossDistance);
                Logger.Log(dist < tossDistance * tossDistance);
                if (dist < tossDistance * tossDistance)
                {
                    Logger.Log("Tossing trash!");
                    GameManager.instance.ShowFree(); // resets trashInHand to 0
                    PlayGarbageSound();
                    
                    thrashRemaining -= GameManager.instance.maxTrashInHand;

                    if (thrashRemaining <= 0)
                    {
                        GameManager.instance.isCleaned = true;
                    }
                }
                Logger.Log("Destination: " + dist);
            }
           
        }

    }

    private void PlayGarbageSound()
    {
        if (garbage == null || garbage.Length == 0)
            return;
            
        int randomIndex = random.Next(0, garbage.Length);
        if (garbage[randomIndex] != null)
        {
            Audio audio = garbage[randomIndex].transform.GetComponent<Audio>();
            if (audio != null)
            {
                audio.Play();
            }
        }
    }

    public void FixedUpdate()
    {

    }

}