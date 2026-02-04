using System.Linq.Expressions;
using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;
using BasilEngine.SceneManagement;
using System;


public class GameManager : Behavior
{
    

    public static GameManager instance;
    public bool TrainPuzzleCompleted = false;
    public bool KitePuzzleCompleted = false;
    public bool isCleaned = false;



    private GameObject player;
    private Rigidbody playerRigidbody;
    private PlayerController3D controller;
    private GameObject ThrashCollect;

    //public bool isHoldingThrash = false;
    public int trashInHand;      // number of trash currently held
    public int maxTrashInHand;   // maximum the player can carry

    public bool hasBag = true; // player starts with a bag
    public bool bagIsFull = false;

    public GameObject maxTrashAudioObject;
    private Audio maxTrashAudio;

    //public GameObject droppedBagPrefab;


    public void Init()
    {
        instance = this;

        ThrashCollect = GameObject.Find("ThrashCollect");
        if (ThrashCollect == null){
            Logger.Warn("ThrashCollect not found; skipping trash visual setup.");
        }
        else
        {
            ThrashCollect.visibility = true; // player starts holding a bag
        }

        player = GameObject.Find("PlayerGroup");

        if (player != null){
            playerRigidbody = player.transform.GetComponent<Rigidbody>();

            if (playerRigidbody == null){
                Logger.Warn("Player Rigidbody not found!");
            }
        
            controller = player.transform.GetComponent<PlayerController3D>();

            if(controller == null){
                Logger.Warn("PlayerController3D not found!");
            }
        }
        else
        {
            Logger.Warn("PlayerGroup not found; skipping player setup.");
        }

        //droppedBagPrefab = GameObject.Find("DroppedTrashBag");

        //if (droppedBagPrefab == null)
        //{
        //    Logger.Warn("DroppedTrashBag prefab not found!");
        //}
        //else
        //{
        //    droppedBagPrefab.visibility = false; // keep template hidden
        //}

        maxTrashAudioObject = GameObject.Find("MengMaxTrashAudio"); // all levels should have
        if (maxTrashAudioObject != null)
        {
            maxTrashAudio = maxTrashAudioObject.transform.GetComponent<Audio>();
            if (maxTrashAudio == null)
                Logger.Warn("MaxTrashAudio object found but Audio component missing!");
        }
        else
        {
            Logger.Warn("MaxTrashAudio object not found!");
        }

    }
    public void Update()
    {
        if (Input.GetKey(KeyCode.Q))
        {
            DropBag();
        }
    }

    public void FixedUpdate()
    {

    }

    public void ShowCollected()
    {
        if (bagIsFull)
            return;

        trashInHand++;
        Logger.Log("Trash collected! Current: " + trashInHand);

        if (trashInHand >= maxTrashInHand)
        {
            trashInHand = maxTrashInHand;
            bagIsFull = true;
            Logger.Log("Bag is full!");

            if (bagIsFull && maxTrashAudio != null)
            {
                maxTrashAudio.Play();
            }
        }

        UpdateBagSize();
        UpdatePlayerSpeed();
    }

    public void ShowFree()
    {
        trashInHand = 0;
        bagIsFull = false;

        Logger.Log("Bag emptied!");

        UpdateBagSize();
        UpdatePlayerSpeed();
    }

    private void UpdateBagSize()
    {
        if (ThrashCollect == null) return;

        if (trashInHand == 0)
        {
            ThrashCollect.transform.scale = new Vector3(0.002f, 0.002f, 0.002f);
            ThrashCollect.transform.position = new Vector3(0f, -0.230f, 0.3f);
        }

        else if (trashInHand <= 2)
        {
            ThrashCollect.transform.scale = new Vector3(0.0021f, 0.0021f, 0.0021f);
            ThrashCollect.transform.position = new Vector3(0f, -0.235f, 0.3f);
        }

        else if (trashInHand <= 4)
        {
            ThrashCollect.transform.scale = new Vector3(0.0023f, 0.0023f, 0.0023f);
            ThrashCollect.transform.position = new Vector3(0f, -0.24f, 0.3f);
        }

        else
        {
            ThrashCollect.transform.scale = new Vector3(0.0025f, 0.0025f, 0.0025f);
            ThrashCollect.transform.position = new Vector3(0f, -0.245f, 0.3f);
        }
    }

    private void UpdatePlayerSpeed()
    {
        if (controller == null) return;

        float minMultiplier = 0.2f; // slowest speed at full bag
        float t = (float)trashInHand / maxTrashInHand;

        controller.speedMultiplier = 1f - (t * (1f - minMultiplier));

        Logger.Log("Speed multiplier: " + controller.speedMultiplier);
    }

    public void DropBag()
    {
        if (trashInHand == 0)
            return;

        trashInHand = 0;
        bagIsFull = false;

        UpdateBagSize();
        UpdatePlayerSpeed();

        Logger.Log("Bag dropped");
    }

    public void SetBagVisibility(bool visible)
    {
        if (ThrashCollect == null) return;

        ThrashCollect.visibility = visible;
    }

    public void DisableControls()
    {
        if (controller != null)
        {
            controller.disabled = true;
        }

        if (playerRigidbody != null)
        {
            playerRigidbody.FreezeX = true;
            playerRigidbody.FreezeZ = true;
        }

    }
    public void EnableControls()
    {
        if (controller != null)
        {
            controller.disabled = false;
        }

        if (playerRigidbody != null)
        {
            playerRigidbody.FreezeX = false;
            playerRigidbody.FreezeZ = false;
        }
    }

    public void CompleteTrainPuzzle()
    {
        TrainPuzzleCompleted = true;
    }

    public void CompleteKitePuzzle()
    {
        KitePuzzleCompleted = true;
    }
}