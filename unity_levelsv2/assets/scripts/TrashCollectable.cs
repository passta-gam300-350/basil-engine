using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;

public class TrashCollectable : Behavior
{
    private GameObject player;
    public float collect_distance = 0.4f;
    private System.Random random = new System.Random();
    private GameObject[] trash;



    public void Init()
    {
        trash = new GameObject[2];
        for (int i = 0; i < trash.Length; i++)
        {
            string objectName = "trash_bag_" + (i + 1);
            trash[i] = GameObject.Find(objectName);
            if (trash[i] == null)
            {
                Logger.Warn("Cannot find game object: " + objectName);
            }
        }
    }

    public void Update()
    {
        if (player == null)
        {
            player =  GameObject.Find("PlayerGroup");
            if (player == null)
            {
                return;
            }
        } 

        float distance = Vector3.DistanceSqr(transform.position, player.transform.position);
        
        if (distance <= collect_distance && Input.GetKey(KeyCode.E))
        {
            if (GameManager.instance.trashInHand >= GameManager.instance.maxTrashInHand)
            {
                Logger.Log("Hands are full!");
                return;
            }

            //TODO: REMOVAL?
            Logger.Log("Player should collect the trash!");
            gameObject.transform.position = new Vector3(1000, 1000, 1000);
            GameObject.Destroy(gameObject);

            GameManager.instance.ShowCollected();
            PlayTrashSound();
        }
        //Logger.Log("Distance" + distance);
    }

    private void PlayTrashSound()
    {
        if (trash == null || trash.Length == 0)
            return;
            
        int randomIndex = random.Next(0, trash.Length);
        if (trash[randomIndex] != null)
        {
            Audio audio = trash[randomIndex].transform.GetComponent<Audio>();
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
