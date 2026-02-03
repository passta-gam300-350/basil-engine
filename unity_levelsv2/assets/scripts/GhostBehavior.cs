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
/// 


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
    private const float Rad2Deg = 180.0f / (float)Math.PI; // Named const variable
    private const float Deg2Rad = (float)Math.PI / 180.0f;
    private float targetYaw = 0.0f;          // Target yaw angle (calculated once when entering pause state)
    private bool rotationComplete = false;   // Flag to track if rotation is done during pause
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
        //rb = GetComponent<Rigidbody>();
        //if (rb == null)
        //{
        //    Logger.Warn("GhostBehavior: CRITICAL - No Rigidbody found! Ghost movement will not work with physics.");
        //    Logger.Warn("GhostBehavior: Add a Rigidbody with MotionType=Kinematic and UseGravity=false");
        //    return;
        //}

        //// Verify Rigidbody is configured correctly
        //if (rb.UseGravity)
        //{
        //    Logger.Warn("GhostBehavior: Rigidbody has gravity enabled! Ghost may fall. Set UseGravity=false in editor.");
        //}

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
    }

    public void Update()
    {
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
        //// Ensure no movement while paused
        //if (rb != null)
        //{
        //    rb.velocity = new Vector3(0, 0, 0);
        //}

        // Rotate while paused (only if not already complete)
        if (!rotationComplete)
        {
            rotationComplete = RotateTowardsNextWaypoint();
        }

        // Always tick the pause timer
        pauseTimer -= Time.deltaTime;

        // Only move on when pause time is done AND rotation is complete
        if (pauseTimer <= 0.0f && rotationComplete)
        {
            AdvanceToNextWaypoint();
            currentState = GhostState.Moving;
        }
    }

    /// <summary>
    /// Smoothly rotate clockwise to face the next waypoint
    /// Uses pre-calculated targetYaw from OnArriveAtWaypoint()
    /// Always rotates clockwise (positive direction) without wrapping
    /// </summary>
    private bool RotateTowardsNextWaypoint()
    {
        // Get current yaw (can be any value from -infinity to +infinity)
        float currentYaw = transform.rotation.y;

        // Calculate how much further we need to rotate
        float remainingRotation = targetYaw - currentYaw;

        // Check if we've reached the target (within tolerance)
        if (Math.Abs(remainingRotation) <= 1.0f)
        {
            Logger.Log($"Rotation complete: Current={currentYaw:F1}° Target={targetYaw:F1}°");
            return true;
        }

        // Calculate rotation step for this frame (always positive = clockwise)
        float maxStep = rotationSpeed * Time.deltaTime;
        float step = remainingRotation;
        if (step > maxStep)
            step = maxStep;

        // Apply rotation step (always add, never subtract = clockwise only)
        float newYaw = currentYaw + step;

        // Debug logging
        Logger.Log($"Rotating clockwise: Current={currentYaw:F1}° Target={targetYaw:F1}° Remaining={remainingRotation:F1}° Step={step:F1}° New={newYaw:F1}°");

        // CRITICAL: Always set X=0 and Z=0 to prevent gimbal lock and axis flipping
        transform.rotation = new Vector3(0f, newYaw, 0f);

        return false;
    }

    /// <summary>
    /// Called when ghost arrives at a waypoint
    /// </summary>
    private void OnArriveAtWaypoint()
    {
        // Start pause at this waypoint
        currentState = GhostState.Paused;
        pauseTimer = GetPauseDuration(currentWaypointIndex);
        rotationComplete = false;  // Reset rotation flag

        //// Stop the rigidbody's velocity to prevent continued movement
        //if (rb != null)
        //{
        //    rb.velocity = new Vector3(0, 0, 0);
        //}

        // Calculate target yaw ONCE when entering pause state
        int nextIndex = currentWaypointIndex + 1;
        if (nextIndex >= waypointCount)
            nextIndex = 0;

        GameObject nextWaypoint = GetWaypoint(nextIndex);
        if (nextWaypoint != null)
        {
            Vector3 toTarget = nextWaypoint.transform.position - transform.position;
            toTarget.y = 0f;

            float distSq = toTarget.x * toTarget.x + toTarget.z * toTarget.z;
            if (distSq >= 0.0001f)
            {
                // Calculate target yaw from world-space direction
                // Atan2(x, z) gives angle from forward axis (0,0,1), returns [-180, 180]
                float desiredYaw = (float)Math.Atan2(toTarget.x, toTarget.z) * Rad2Deg;

                // Get current yaw (can be any value: 360, 720, -360, etc.)
                float currentYaw = transform.rotation.y;

                // Wrap currentYaw to [-180, 180] to compare with desiredYaw
                float wrappedCurrentYaw = currentYaw;
                while (wrappedCurrentYaw > 180f) wrappedCurrentYaw -= 360f;
                while (wrappedCurrentYaw < -180f) wrappedCurrentYaw += 360f;

                // Calculate how much we need to add to currentYaw to reach desiredYaw clockwise
                // If desiredYaw is "behind" wrappedCurrentYaw (counter-clockwise), add 360
                targetYaw = desiredYaw;
                if (desiredYaw <= wrappedCurrentYaw)
                {
                    targetYaw = desiredYaw + 360f;
                }

                // Now adjust targetYaw to be relative to the actual currentYaw (not wrapped)
                // We want targetYaw to always be > currentYaw for clockwise rotation
                float offset = currentYaw - wrappedCurrentYaw;  // This is the multiple of 360
                targetYaw += offset;

                Logger.Log($"GhostBehavior: Current={currentYaw:F1}° Wrapped={wrappedCurrentYaw:F1}° Desired={desiredYaw:F1}° Target={targetYaw:F1}°");
            }
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

    // ========================================================================
    // TRIGGER COLLISION CALLBACKS (Unity-style)
    // ========================================================================
    // These methods are automatically called by the physics system when
    // another collider enters, stays in, or exits this ghost's trigger zone.
    // Make sure the ghost has a collider component with isTrigger = true!
    // ========================================================================

    /// <summary>
    /// Called when another collider enters the ghost's trigger zone
    /// </summary>
    //public void OnTriggerEnter()
    //{
    //    // Check if the colliding object is the player
    //    if (other.name == "Player" || other.name == "PlayerGroup")
    //    {
    //        Logger.Log($"GhostBehavior: Player entered ghost trigger zone!");

    //        // Example interactions:
    //        // - Play a sound effect
    //        // - Trigger a jumpscare animation
    //        // - Load a different scene (game over)
    //        // - Damage the player
    //        // - Enable a UI prompt

    //        // Example: Load game over scene
    //        // Scene.LoadScene(0);
    //    }
    //}

    /// <summary>
    /// Called every frame while another collider stays in the ghost's trigger zone
    /// </summary>
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

    ///// <summary>
    ///// Called when another collider exits the ghost's trigger zone
    ///// </summary>
    //public void OnTriggerExit(GameObject other)
    //{
    //    // Check if the player left the trigger zone
    //    if (other.name == "Player" || other.name == "PlayerGroup")
    //    {
    //        Logger.Log($"GhostBehavior: Player left ghost trigger zone.");

    //        // Example interactions:
    //        // - Stop playing sounds
    //        // - Hide UI warnings
    //        // - Resume normal gameplay
    //    }
    //}
}
