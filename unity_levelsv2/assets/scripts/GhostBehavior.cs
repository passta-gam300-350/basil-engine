using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;
using System.Security.AccessControl;

/// <summary>
/// Ghost patrol behavior with waypoint-based movement and trigger collision detection.
///
/// REQUIRED COMPONENTS (must be added in editor):
/// ================================================
/// 1. RigidBodyComponent
///    - Motion Type: KINEMATIC (for script-controlled movement)
///    - Use Gravity: FALSE (ghost shouldn't fall)
///    - Drag: 0 (no friction)
///
/// 2. Collider (BoxCollider, SphereCollider, or CapsuleCollider)
///    - isTrigger: TRUE (to pass through walls)
///    - Size: Large enough to detect player proximity
///
/// WHY RIGIDBODY IS REQUIRED:
/// - Collider-only creates a Static Jolt body that CANNOT move
/// - Without RigidBody, transform changes don't sync to Jolt physics
/// - Kinematic RigidBody allows MovePosition() while ignoring gravity/forces
/// - Trigger callbacks only work if Jolt knows the ghost's current position
///
/// BEHAVIOR:
/// - Patrols between waypoints with configurable pause durations
/// - Smoothly rotates to face movement direction
/// - Triggers OnTriggerEnter/Stay/Exit when player enters detection zone
/// </summary>
public class GhostBehavior : Behavior
{
    // PUBLIC CONFIGURATION - Set these in the editor
    // Waypoint GameObjects - leave unused ones as null
    public GameObject waypoint1;
    public GameObject waypoint2;
    public GameObject waypoint3;
    public GameObject waypoint4;
    public GameObject waypoint5;
    public GameObject waypoint6;
    public GameObject waypoint7;
    public GameObject waypoint8;
    public GameObject waypoint9;
    public GameObject waypoint10;

    // Pause durations at each waypoint (in seconds)
    public float pauseDuration1 = 1.0f;
    public float pauseDuration2 = 1.0f;
    public float pauseDuration3 = 1.0f;
    public float pauseDuration4 = 1.0f;
    public float pauseDuration5 = 1.0f;
    public float pauseDuration6 = 1.0f;
    public float pauseDuration7 = 1.0f;
    public float pauseDuration8 = 1.0f;
    public float pauseDuration9 = 1.0f;
    public float pauseDuration10 = 1.0f;

    // Movement settings
    public float moveSpeed = 2.0f;           // Movement speed (units per second)
    public float rotationSpeed = 360.0f;     // Rotation speed in degrees per second
    public float arrivalThreshold = 0.1f;    // Distance to waypoint to consider "arrived"

    // PRIVATE STATE
    private int currentWaypointIndex = 0;    // Which waypoint we're targeting (0-based)
    private int waypointCount = 0;           // Total number of valid waypoints
    private GhostState currentState = GhostState.Moving;
    private float pauseTimer = 0.0f;         // Timer for pause duration
    private Rigidbody rb;                    // Rigidbody component for physics-based movement

    public float ghostRadius;
    private PlayerController3D player;
    private GameObject playerObj;
    private bool playerInside = false;

    private bool activated = false;

    private enum GhostState
    {
        Moving,     // Moving towards waypoint
        Paused      // Waiting at waypoint
    }

    public void Init()
    {
        // IMPORTANT: Ghost MUST have a Rigidbody for movement to work with Jolt physics
        // Without RigidBody, the Jolt body stays static and collision detection breaks
        // Required setup in editor:
        //   - RigidBodyComponent: Motion Type = Kinematic, UseGravity = false
        //   - Collider: isTrigger = true (to pass through walls)
        rb = GetComponent<Rigidbody>();
        if (rb == null)
        {
            Logger.Warn("GhostBehavior: CRITICAL - No Rigidbody found! Ghost movement will not work with physics.");
            Logger.Warn("GhostBehavior: Add a Rigidbody with MotionType=Kinematic and UseGravity=false");
            return;
        }

        // Verify Rigidbody is configured correctly
        if (rb.UseGravity)
        {
            Logger.Warn("GhostBehavior: Rigidbody has gravity enabled! Ghost may fall. Set UseGravity=false in editor.");
        }

        waypoint1 = GameObject.Find("1");
        waypoint2 = GameObject.Find("2");
        waypoint3 = GameObject.Find("3");
        waypoint4 = GameObject.Find("4");


        // Count how many waypoints are assigned
        waypointCount = CountWaypoints();

        if (waypointCount == 0)
        {
            Logger.Warn("GhostBehavior: No waypoints assigned! Ghost will not move.");
            return;
        }

        // Start at first waypoint
        currentWaypointIndex = 0;
        currentState = GhostState.Moving;
        pauseTimer = 0.0f;
        Logger.Log($"GhostBehavior initialized with {waypointCount} waypoints.");

        playerObj = GameObject.Find("PlayerGroup");

        if (playerObj != null)
        {
            player = playerObj.transform.GetComponent<PlayerController3D>();
        }
        else
        {
            Logger.Warn("GhostBehavior: PlayerGroup not found");
        }

        gameObject.visibility = false;   // start hidden
        activated = false;
    }

    public void Update()
    {

        if (!activated)
            return;

        // Don't do anything if no waypoints configured
        if (waypointCount == 0)
        {
            return;
        }

        switch (currentState)
        {
            case GhostState.Moving:
                UpdateMovement();
                break;

            case GhostState.Paused:
                UpdatePause();
                break;
        }

        UpdatePlayerDistance();
    }

    public void FixedUpdate()
    {
        // Not using fixed update for now
    }

    /// <summary>
    /// Count how many waypoints are assigned
    /// </summary>
    private int CountWaypoints()
    {
        int count = 0;
        if (waypoint1 != null) count++;
        if (waypoint2 != null) count++;
        if (waypoint3 != null) count++;
        if (waypoint4 != null) count++;
        if (waypoint5 != null) count++;
        if (waypoint6 != null) count++;
        if (waypoint7 != null) count++;
        if (waypoint8 != null) count++;
        if (waypoint9 != null) count++;
        if (waypoint10 != null) count++;
        return count;
    }

    /// <summary>
    /// Get waypoint at specified index (0-based)
    /// </summary>
    private GameObject GetWaypoint(int index)
    {
        switch (index)
        {
            case 0: return waypoint1;
            case 1: return waypoint2;
            case 2: return waypoint3;
            case 3: return waypoint4;
            case 4: return waypoint5;
            case 5: return waypoint6;
            case 6: return waypoint7;
            case 7: return waypoint8;
            case 8: return waypoint9;
            case 9: return waypoint10;
            default: return null;
        }
    }

    /// <summary>
    /// Get pause duration at specified index (0-based)
    /// </summary>
    private float GetPauseDuration(int index)
    {
        switch (index)
        {
            case 0: return pauseDuration1;
            case 1: return pauseDuration2;
            case 2: return pauseDuration3;
            case 3: return pauseDuration4;
            case 4: return pauseDuration5;
            case 5: return pauseDuration6;
            case 6: return pauseDuration7;
            case 7: return pauseDuration8;
            case 8: return pauseDuration9;
            case 9: return pauseDuration10;
            default: return 1.0f;
        }
    }

    /// <summary>
    /// Update movement towards current waypoint
    /// </summary>
    private void UpdateMovement()
    {
        GameObject targetWaypoint = GetWaypoint(currentWaypointIndex);
        if (targetWaypoint == null)
        {
            Logger.Warn($"GhostBehavior: Waypoint at index {currentWaypointIndex} is null! Skipping...");
            AdvanceToNextWaypoint();
            return;
        }

        Vector3 targetPosition = targetWaypoint.transform.position;
        Vector3 currentPosition = transform.position;

        // Calculate direction and distance
        Vector3 direction = targetPosition - currentPosition;
        float distance = direction.Magnitude();

        // Check if we've arrived at the waypoint
        if (distance <= arrivalThreshold)
        {
            OnArriveAtWaypoint();
            return;
        }

        // Move towards the target waypoint (no rotation while moving)
        Vector3 normalizedDirection = direction.Normalize();
        Vector3 newPosition;

        if (moveSpeed * Time.deltaTime >= distance)
        {
            // We'll reach or overshoot the target this frame, so just snap to it
            newPosition = targetPosition;
        }
        else
        {
            // Move towards target at moveSpeed units per second
            newPosition = currentPosition + (normalizedDirection * moveSpeed * Time.deltaTime);
        }

        // Set position directly
        transform.position = newPosition;
    }

    /// <summary>
    /// Update pause timer at waypoint and rotate to face next waypoint
    /// </summary>
    private void UpdatePause()
    {
        // Ensure velocity stays at zero while paused
        if (rb != null)
        {
            rb.velocity = new Vector3(0, 0, 0);
        }

        // Rotate to face the next waypoint while paused
        RotateTowardsNextWaypoint();

        // Update pause timer
        pauseTimer -= Time.deltaTime;

        if (pauseTimer <= 0.0f)
        {
            // Pause finished, move to next waypoint
            AdvanceToNextWaypoint();
            currentState = GhostState.Moving;
        }
    }

    /// <summary>
    /// Smoothly rotate to face the next waypoint
    /// </summary>
    private void RotateTowardsNextWaypoint()
    {
        // Calculate which waypoint is next
        int nextWaypointIndex = currentWaypointIndex + 1;
        if (nextWaypointIndex >= waypointCount)
        {
            nextWaypointIndex = 0; // Loop back to first waypoint
        }

        GameObject nextWaypoint = GetWaypoint(nextWaypointIndex);
        if (nextWaypoint == null)
        {
            return; // Can't rotate if next waypoint doesn't exist
        }

        // Calculate direction to next waypoint
        Vector3 targetPosition = nextWaypoint.transform.position;
        Vector3 currentPosition = transform.position;
        Vector3 direction = targetPosition - currentPosition;

        // Only rotate if next waypoint is far enough away
        if (direction.Magnitude() < 0.1f)
        {
            return;
        }

        Vector3 normalizedDirection = direction.Normalize();

        // Calculate target yaw angle in degrees
        // Math.Atan2 returns radians, so convert to degrees (* 180/PI)
        float targetYaw = (float)(Math.Atan2(normalizedDirection.x, normalizedDirection.z) * (180.0 / Math.PI));

        // Get current rotation
        Vector3 currentRotation = transform.rotation;
        float currentYaw = currentRotation.y;

        // Calculate angle difference with wrapping
        float angleDifference = targetYaw - currentYaw;
        while (angleDifference > 180.0f) angleDifference -= 360.0f;
        while (angleDifference < -180.0f) angleDifference += 360.0f;

        // Only rotate if difference is significant (> 1 degree)
        if (Math.Abs(angleDifference) > 1.0f)
        {
            // Calculate rotation step for this frame
            float rotationStep = rotationSpeed * Time.deltaTime;

            // Apply rotation
            float newYaw;
            if (Math.Abs(angleDifference) <= rotationStep)
            {
                // Close enough, snap to target
                newYaw = targetYaw;
            }
            else
            {
                // Rotate towards target
                newYaw = currentYaw + Math.Sign(angleDifference) * rotationStep;
            }

            // Normalize angle to [0, 360) range
            while (newYaw < 0.0f) newYaw += 360.0f;
            while (newYaw >= 360.0f) newYaw -= 360.0f;

            // Apply rotation (only change Y axis)
            transform.rotation = new Vector3(currentRotation.x, newYaw, currentRotation.z);
        }
    }

    /// <summary>
    /// Called when ghost arrives at a waypoint
    /// </summary>
    private void OnArriveAtWaypoint()
    {
        // Start pause at this waypoint
        currentState = GhostState.Paused;
        pauseTimer = GetPauseDuration(currentWaypointIndex);

        // Stop the rigidbody's velocity to prevent continued movement
        if (rb != null)
        {
            rb.velocity = new Vector3(0, 0, 0);
        }

        Logger.Log($"GhostBehavior: Arrived at waypoint {currentWaypointIndex + 1}, pausing for {pauseTimer} seconds.");
    }

    /// <summary>
    /// Move to the next waypoint in the sequence
    /// </summary>
    private void AdvanceToNextWaypoint()
    {
        currentWaypointIndex++;

        // Loop back to start if we've reached the end
        if (currentWaypointIndex >= waypointCount)
        {
            currentWaypointIndex = 0;
        }

        Logger.Log($"GhostBehavior: Moving to waypoint {currentWaypointIndex + 1}.");
    }

    private void UpdatePlayerDistance()
    {
        if (player == null || playerObj == null)
            return;

        float distSqr = Vector3.DistanceSqr(
            transform.position,
            playerObj.transform.position
        );

        float radiusSqr = ghostRadius * ghostRadius;

        if (distSqr <= radiusSqr)
        {
            if (!playerInside)
            {
                playerInside = true;

                player.interactionsLocked = true;
                player.wantsToCollect = false;
                player.wantsToMop = false;
            }
        }
        else
        {
            if (playerInside)
            {
                playerInside = false;

                player.interactionsLocked = false;

            }
        }
    }

    public void ActivateGhost()
    {
        gameObject.visibility = true;
        activated = true;

        currentWaypointIndex = 0;
        currentState = GhostState.Moving;

    }


    // ========================================================================
    // TRIGGER COLLISION CALLBACKS (Unity-style)
    // ========================================================================
    // These methods are automatically called by the physics system when
    // another collider enters, stays in, or exits this ghost's trigger zone.
    // Make sure the ghost has a collider component with isTrigger = true!
    // ========================================================================

    // Called when another collider enters the ghost's trigger zone
    //public void OnTriggerEnter(GameObject other)
    //{
    //    // Check if the colliding object is the player
    //    if (other.name == "Player" || other.name == "PlayerGroup")
    //    {
    //        Logger.Log($"GhostBehavior: Player entered ghost trigger zone!");
    //    }
    //}

    // Called every frame while another collider stays in the ghost's trigger zone
    //public void OnTriggerStay(GameObject other)
    //{
    //    // Check if player is still in range
    //    if (other.name == "Player" || other.name == "PlayerGroup")
    //    {
    //        // This is called every frame while player is inside the trigger
    //        // Use this for continuous effects like:
    //        // - Draining player health over time
    //        // - Playing ambient sounds
    //        // - Showing warning UI

    //        // Example: Log every 60 frames (roughly once per second at 60fps)
    //        // Note: You'd need to add a frame counter variable for this
    //    }
    //}


    // Called when another collider exits the ghost's trigger zone
    //public void OnTriggerExit(GameObject other)
    //{
    //    if (other.name == "Player" || other.name == "PlayerGroup")
    //    {
    //        Logger.Log($"GhostBehavior: Player left ghost trigger zone.");

    //    }
    //}
}
