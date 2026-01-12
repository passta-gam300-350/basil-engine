using System.Linq.Expressions;
using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;
using BasilEngine.SceneManagement;


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
    public int trashInHand = 0;      // number of trash currently held
    public int maxTrashInHand = 5;   // maximum the player can carry

    
    public void Init()
    {
        instance = this;

        ThrashCollect = GameObject.Find("ThrashCollect");
        if (ThrashCollect == null){
            Logger.Warn("ThrashCollect not found; skipping trash visual setup.");
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

    }
    public void Update()
    {

        if (player != null && isCleaned && KitePuzzleCompleted && TrainPuzzleCompleted)
        {
            Logger.Log("Congratz: Move to Kite Level!");
            Scene.LoadScene(3);
        }
    }

    public void FixedUpdate()
    {

    }

    public void ShowCollected()
    {
        trashInHand = Math.Min(trashInHand + 1, maxTrashInHand);
        Logger.Log("Trash collected! Current number of trash: " + trashInHand);

        // Show visual only if player has collected all 5 trash
        if (trashInHand >= maxTrashInHand && ThrashCollect != null)
        {
            ThrashCollect.visibility = true;
            Logger.Log("Trash bag ready!");
        }
        
        //isHoldingThrash = true;
    }

    public void ShowFree()
    {
        if (trashInHand > 0)
        {
            Logger.Log("Tossing all trash");
            trashInHand = 0; // toss all trash at once

            if(ThrashCollect != null){

            ThrashCollect.visibility = false;
            }
        }

        //isHoldingThrash = false;
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